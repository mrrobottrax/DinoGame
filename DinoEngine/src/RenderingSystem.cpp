#include "pch.h"

#include "GameDllSystem.h"
#include "RenderingSystem.h"
#include "UISystem.h"
#include "WindowSystem.h"
#include "asset_types.h"

DINO_API IRenderingSystem *g_IRenderingSystem = &g_RenderingSystem;

constexpr UINT k_SwapChainFlags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
constexpr DXGI_FORMAT k_SwapChainFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

#if defined(_DEBUG)
static void __stdcall d3d12_message_callback(D3D12_MESSAGE_CATEGORY Category,
                                             D3D12_MESSAGE_SEVERITY Severity,
                                             D3D12_MESSAGE_ID ID,
                                             LPCSTR pDescription,
                                             void *pContext) {
  (void)Category;
  (void)Severity;
  (void)ID;
  (void)pDescription;
  (void)pContext;

  switch (Severity) {
  case D3D12_MESSAGE_SEVERITY_WARNING:
    console_warn("D3D12: %s", pDescription);
    break;

  case D3D12_MESSAGE_SEVERITY_ERROR:
    console_error("D3D12: %s", pDescription);
    break;

  default:
    console_log("D3D12: %s", pDescription);
    break;
  }
}
#endif //  defined(_DEBUG)

void RenderingSystem::start() {
  ASSERT(g_GameDllSystem.is_initialized());
  ASSERT(g_WindowSystem.get_hWnd() != NULL);

  GameInfo &game = g_GameDllSystem.GameInfo;

#if defined(_DEBUG)
  // enable the D3D12 debug layer
  {
    ComPtr<ID3D12Debug5> pDebugController;
    ASSERT_WIN_ALWAYS(D3D12GetDebugInterface(IID_PPV_ARGS(&pDebugController)));
    pDebugController->EnableDebugLayer();
    pDebugController->SetEnableAutoName(true);
    pDebugController->SetEnableGPUBasedValidation(true);
    pDebugController->SetEnableSynchronizedCommandQueueValidation(true);
    pDebugController->SetGPUBasedValidationFlags(
        D3D12_GPU_BASED_VALIDATION_FLAGS_NONE);
  }
#endif

  ComPtr<IDXGIFactory6> pDxgiFactory;
  UINT createFactoryFlags = 0;

#if defined(_DEBUG)
  createFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

  ASSERT_WIN_ALWAYS(
      CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&pDxgiFactory)));

  BOOL allowsTearing = FALSE;
  ASSERT_WIN_ALWAYS(
      pDxgiFactory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING,
                                        &allowsTearing, sizeof(allowsTearing)));
  ASSERT_ALWAYS(allowsTearing);

  create_device(pDxgiFactory.Get());

#if defined(_DEBUG)
  // custom message callback
  ComPtr<ID3D12InfoQueue1> infoQueue;
  ASSERT_WIN_ALWAYS(m_pDevice->QueryInterface(IID_PPV_ARGS(&infoQueue)));

#pragma warning(push)
#pragma warning(disable : 6387)
  DWORD callbackCookie = 0;
  ASSERT_WIN_ALWAYS(infoQueue->RegisterMessageCallback(
      d3d12_message_callback, D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr,
      &callbackCookie));
#pragma warning(pop)

#endif

  m_hGPUStallEvent = CreateEventEx(NULL, L"Big GPU stall", 0, EVENT_ALL_ACCESS);
  ASSERT_WIN_EXP_ALWAYS(m_hGPUStallEvent != NULL);

  ASSERT_WIN_ALWAYS(m_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                                           IID_PPV_ARGS(&m_GPUStallFence)));

  D3D12_COMMAND_QUEUE_DESC commandQueueDesc{
      .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
      .Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH,
      .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
      .NodeMask = 0,
  };
  ASSERT_WIN_ALWAYS(m_pDevice->CreateCommandQueue(
      &commandQueueDesc, IID_PPV_ARGS(&m_CommandQueue)));

  // get increment sizes
  m_RtvDescriptorIncrementSize = m_pDevice->GetDescriptorHandleIncrementSize(
      D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  m_SrvCbvUabDescriptorIncrementSize =
      m_pDevice->GetDescriptorHandleIncrementSize(
          D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

  // create swapchain
  {
    ComPtr<IDXGISwapChain1> swapChain1;
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc{
        .Width = 0,
        .Height = 0,
        .Format = k_SwapChainFormat,
        .Stereo = FALSE,
        .SampleDesc =
            {
                .Count = 1,
                .Quality = 0,
            },
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = k_FramesInFlight,
        .Scaling = DXGI_SCALING_STRETCH,
        .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
        .AlphaMode = DXGI_ALPHA_MODE_IGNORE,
        .Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING,
    };
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFullscreenDesc{
        .RefreshRate = 0,
        .ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
        .Scaling = DXGI_MODE_SCALING_STRETCHED,
        .Windowed = TRUE,
    };
    ASSERT_WIN_ALWAYS(pDxgiFactory->CreateSwapChainForHwnd(
        m_CommandQueue.Get(), g_WindowSystem.get_hWnd(), &swapChainDesc,
        &swapChainFullscreenDesc, NULL, &swapChain1));
    ASSERT_WIN_ALWAYS(swapChain1.As(&m_SwapChain));

    DXGI_SWAP_CHAIN_DESC1 swapChainDescRetrieved;
    m_SwapChain->GetDesc1(&swapChainDescRetrieved);
    m_SwapChainW = swapChainDescRetrieved.Width;
    m_SwapChainH = swapChainDescRetrieved.Height;

    ASSERT_WIN_ALWAYS(pDxgiFactory->MakeWindowAssociation(
        g_WindowSystem.get_hWnd(), DXGI_MWA_NO_ALT_ENTER));

    // create rtv descriptor heap
    {
      D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{
          .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
          .NumDescriptors = k_FramesInFlight,
          .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
          .NodeMask = 0,
      };
      ASSERT_WIN_ALWAYS(m_pDevice->CreateDescriptorHeap(
          &descriptorHeapDesc, IID_PPV_ARGS(&m_RTVDescriptorHeap)));
    }

    // create srv descriptor heap
    {
      D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{
          .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
          .NumDescriptors = k_FramesInFlight,
          .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
          .NodeMask = 0,
      };
      ASSERT_WIN_ALWAYS(m_pDevice->CreateDescriptorHeap(
          &descriptorHeapDesc, IID_PPV_ARGS(&m_RTVasSRVDescriptorHeap)));
    }
  }

  // create staging data
  {
    ASSERT_WIN_ALWAYS(m_pDevice->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_pStagingAllocator)));

    ASSERT_WIN_ALWAYS(m_pDevice->CreateCommandList1(
        0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE,
        IID_PPV_ARGS(&m_pStagingList)));

    ASSERT_WIN_ALWAYS(m_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                                             IID_PPV_ARGS(&m_pStagingFence)));

    m_StagingFenceEvent = CreateEventEx(NULL, NULL, 0, EVENT_ALL_ACCESS);
    ASSERT_ALWAYS(m_StagingFenceEvent != NULL);

    m_StagingFenceValue = 0;

    m_StagingHeapSize = game.GPUStagingBufferSize;
    D3D12_HEAP_DESC stagingHeapDesc{
        .SizeInBytes = m_StagingHeapSize,
        .Properties =
            {
                .Type = D3D12_HEAP_TYPE_UPLOAD,
            },
    };
    ASSERT_WIN_ALWAYS(
        m_pDevice->CreateHeap(&stagingHeapDesc, IID_PPV_ARGS(&m_StagingHeap)));

    D3D12_RESOURCE_DESC stagingBufferDesc{
        .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
        .Alignment = 0,
        .Width = m_StagingHeapSize,
        .Height = 1,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = DXGI_FORMAT_UNKNOWN,
        .SampleDesc = {1, 0},
        .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        .Flags = D3D12_RESOURCE_FLAG_NONE,
    };
    ASSERT_WIN_ALWAYS(m_pDevice->CreatePlacedResource(
        m_StagingHeap.Get(), 0, &stagingBufferDesc,
        D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr,
        IID_PPV_ARGS(&m_StagingResource)));

    ASSERT_WIN_ALWAYS(m_StagingResource->Map(0, nullptr, &m_StagingHeapMap));
  }

  // create frame data
  {
    for (UINT i = 0; i < k_FramesInFlight; ++i) {
      FrameData &fd = m_FrameData[i];
      ASSERT_WIN_ALWAYS(m_pDevice->CreateCommandAllocator(
          D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&fd.CommandAllocator)));

      ASSERT_WIN_ALWAYS(m_pDevice->CreateCommandList1(
          0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE,
          IID_PPV_ARGS(&fd.CommandList)));

      ASSERT_WIN_ALWAYS(m_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                                               IID_PPV_ARGS(&fd.Fence)));

      fd.FenceEvent = CreateEventEx(NULL, NULL, 0, EVENT_ALL_ACCESS);
      ASSERT_ALWAYS(fd.FenceEvent != NULL);

      fd.FenceValue = 0;
    }

    create_backbuffer_data();
  }

  // create descriptor heap
  {
    m_DescriptorCapacity = game.GPUMaxResources;

    D3D12_DESCRIPTOR_HEAP_DESC staticDescriptorHeapDesc{
        .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        .NumDescriptors = m_DescriptorCapacity,
        .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
    };
    ASSERT_WIN_ALWAYS(m_pDevice->CreateDescriptorHeap(
        &staticDescriptorHeapDesc, IID_PPV_ARGS(&m_DescriptorHeap)));
  }

  // create data heap
  {
    m_DataHeapCapacity = game.GPUDataBufferSize;
    D3D12_HEAP_DESC staticHeapDesc{
        .SizeInBytes = m_DataHeapCapacity,
        .Properties =
            {
                .Type = D3D12_HEAP_TYPE_DEFAULT,
            },
    };
    ASSERT_WIN_ALWAYS(
        m_pDevice->CreateHeap(&staticHeapDesc, IID_PPV_ARGS(&m_DataHeap)));
  }

  // create resources array
  {
    m_ResourceCapacity = game.GPUMaxResources;
    m_Resources = (ID3D12Resource **)malloc(m_ResourceCapacity *
                                            sizeof(ID3D12Resource *));
    ASSERT_ALWAYS(m_Resources);
  }

  m_IsInitialized = true;
}

void RenderingSystem::stop() {
  m_IsInitialized = false;
  wait_idle();

  // destroy resources
  for (uint32_t i = 0; i < m_ResourceCount; ++i) {
    m_Resources[i]->Release();
  }
  free(m_Resources);
  m_Resources = 0;
  m_ResourceCount = 0;
  m_ResourceCapacity = 0;

  // destroy data heap
  m_DataHeap.Reset();
  m_DataHeapCapacity = 0;
  m_DataHeapOffset = 0;

  // destroy descriptor heap
  m_DescriptorHeap.Reset();
  m_DescriptorCapacity = 0;
  m_DescriptorCount = 0;

  // destroy frame data
  for (UINT i = 0; i < k_FramesInFlight; ++i) {
    FrameData &fd = m_FrameData[i];
    CloseHandle(fd.FenceEvent);
    fd.Fence.Reset();
    fd.Backbuffer.Reset();
    fd.CommandList.Reset();
    fd.CommandAllocator.Reset();
  }

  // destroy staging heap
  m_StagingHeap.Reset();
  m_StagingResource.Reset();
  m_StagingHeapSize = 0;
  m_StagingHeapMap = 0;
  m_pStagingFence.Reset();
  m_pStagingList.Reset();
  m_pStagingAllocator.Reset();
  CloseHandle(m_StagingFenceEvent);
  m_StagingFenceValue = 0;

  // destroy higher level stuff
  m_SwapChain.Reset();
  m_CommandQueue.Reset();
  m_RTVDescriptorHeap.Reset();
  m_RTVasSRVDescriptorHeap.Reset();
  m_GPUStallFence.Reset();

  CloseHandle(m_hGPUStallEvent);

#if defined(_DEBUG)
  ComPtr<ID3D12DebugDevice> debugDevice;
  ASSERT_WIN(m_pDevice->QueryInterface(IID_PPV_ARGS(&debugDevice)));
#endif
  m_pDevice.Reset();
#if defined(_DEBUG)
  ASSERT_WIN(debugDevice->ReportLiveDeviceObjects(
      D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL));
#endif
}

void RenderingSystem::create_device(IDXGIFactory6 *pDxgiFactory) {
  console_log("GPU Adapters:");
  while (true) {
    UINT i = 0;
    ComPtr<IDXGIAdapter4> pBestAdapter;
    ComPtr<IDXGIAdapter4> pAdapter;
    while (pDxgiFactory->EnumAdapterByGpuPreference(
               i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
               IID_PPV_ARGS(&pAdapter)) != DXGI_ERROR_NOT_FOUND) {
      DXGI_ADAPTER_DESC3 desc;
      ASSERT_WIN_ALWAYS(pAdapter->GetDesc3(&desc));

      if (!pBestAdapter) {
        console_log("\t%ls (picked)", desc.Description);
        pBestAdapter = pAdapter;
        pAdapter.Reset();
        ++i; // increment so that if we try again, it's on the next one
        break;
      } else {
        console_log("\t%ls", desc.Description);
      }

      pAdapter.Reset();
      ++i;
    }

    if (pBestAdapter == nullptr) {
      CRASH("Failed to find a suitable adapter.");
    }

    if (!SUCCEEDED(D3D12CreateDevice(pBestAdapter.Get(), D3D_FEATURE_LEVEL_12_1,
                                     IID_PPV_ARGS(&m_pDevice)))) {
      console_warn(
          "Adapter does not support D3D_FEATURE_LEVEL_12_1. Trying again:");
      pBestAdapter.Reset();

      // deliberately don't reset i.
    } else {
      break;
    }
  }
}

void RenderingSystem::create_backbuffer_data() {
  for (UINT i = 0; i < k_FramesInFlight; ++i) {
    FrameData &fd = m_FrameData[i];

    ASSERT_WIN_ALWAYS(m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&fd.Backbuffer)));

    D3D12_RENDER_TARGET_VIEW_DESC rtViewDesc{
        .Format = k_SwapChainFormat,
        .ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
        .Texture2D =
            {
                .MipSlice = 0,
                .PlaneSlice = 0,
            },
    };

    D3D12_SHADER_RESOURCE_VIEW_DESC srViewDesc{
        .Format = k_SwapChainFormat,
        .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
        .Texture2D =
            {
                .MostDetailedMip = 0,
                .MipLevels = 1,
                .PlaneSlice = 0,
                .ResourceMinLODClamp = 0,
            },
    };

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle =
        m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += m_RtvDescriptorIncrementSize * i;

    D3D12_CPU_DESCRIPTOR_HANDLE srvHandle =
        m_RTVasSRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    srvHandle.ptr += m_RtvDescriptorIncrementSize * i;

    m_pDevice->CreateRenderTargetView(fd.Backbuffer.Get(), &rtViewDesc,
                                      rtvHandle);

    m_pDevice->CreateShaderResourceView(fd.Backbuffer.Get(), &srViewDesc,
                                        srvHandle);
  }
}

void RenderingSystem::frame() {
  if (m_SwapChainW == 0 || m_SwapChainH == 0) {
    return;
  }

  // SETUP
  UINT iFrame = m_SwapChain->GetCurrentBackBufferIndex();

  FrameData &fd = m_FrameData[iFrame];
  if (fd.Fence->GetCompletedValue() < fd.FenceValue) {
    fd.Fence->SetEventOnCompletion(fd.FenceValue, fd.FenceEvent);
    WaitForSingleObject(fd.FenceEvent, INFINITE);
  }

  ASSERT(fd.CommandAllocator);
  ASSERT(fd.CommandList);

  ASSERT_WIN_ALWAYS(fd.CommandAllocator->Reset());
  ASSERT_WIN_ALWAYS(fd.CommandList->Reset(fd.CommandAllocator.Get(), NULL));

  D3D12_RESOURCE_BARRIER unknownToRenderTargetBarrier{
      .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
      .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
      .Transition{
          .pResource = fd.Backbuffer.Get(),
          .Subresource = 0,
          .StateBefore = D3D12_RESOURCE_STATE_COMMON,
          .StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET,
      }};
  fd.CommandList->ResourceBarrier(1, &unknownToRenderTargetBarrier);

  D3D12_CPU_DESCRIPTOR_HANDLE rtvCpuHandle =
      m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
  rtvCpuHandle.ptr += m_RtvDescriptorIncrementSize * iFrame;

  float color[4] = {0, 0, 0, 1};
  fd.CommandList->ClearRenderTargetView(rtvCpuHandle, color, 0, NULL);

  fd.CommandList->OMSetRenderTargets(1, &rtvCpuHandle, TRUE, nullptr);

  D3D12_RESOURCE_DESC backBufferDesc =
      m_FrameData[iFrame].Backbuffer->GetDesc();
  ASSERT(backBufferDesc.Width <= UINT_MAX);

  // RENDER UI
  g_UISystem.add_render_commands(fd.CommandList.Get(),
                                 (uint32_t)backBufferDesc.Width,
                                 backBufferDesc.Height);

  // PRESENT
  D3D12_RESOURCE_BARRIER renderTargetToPresentBarrier{
      .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
      .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
      .Transition{
          .pResource = fd.Backbuffer.Get(),
          .Subresource = 0,
          .StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET,
          .StateAfter = D3D12_RESOURCE_STATE_PRESENT,
      }};
  fd.CommandList->ResourceBarrier(1, &renderTargetToPresentBarrier);

  ASSERT_WIN_ALWAYS(fd.CommandList->Close());
  ID3D12CommandList *ppCommandLists[] = {fd.CommandList.Get()};
  m_CommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
  fd.FenceValue++;
  ASSERT_WIN_ALWAYS(m_CommandQueue->Signal(fd.Fence.Get(), fd.FenceValue));

  DXGI_PRESENT_PARAMETERS presentParameters{
      .DirtyRectsCount = 0,
      .pDirtyRects = nullptr,
      .pScrollRect = nullptr,
      .pScrollOffset = nullptr,
  };
  ASSERT_WIN_ALWAYS(m_SwapChain->Present1(0, DXGI_PRESENT_ALLOW_TEARING,
                                           &presentParameters));
}

void RenderingSystem::try_resize(unsigned int w, unsigned int h) {
  if (!m_SwapChain)
    return;

  if (m_SwapChainW == w && m_SwapChainH == h)
    return;

  console_log("SIZE: %u, %u", w, h);
  if (w == 0 || h == 0) {
    m_SwapChainW = m_SwapChainH = 0;
    return;
  }

  wait_idle();

  for (UINT i = 0; i < k_FramesInFlight; ++i) {
    FrameData &fd = m_FrameData[i];
    fd.Backbuffer.Reset();
  }

  ASSERT_WIN_ALWAYS(m_SwapChain->ResizeBuffers(
      k_FramesInFlight, w, h, k_SwapChainFormat, k_SwapChainFlags));

  create_backbuffer_data();

  D3D12_RESOURCE_DESC backbufferDesc = m_FrameData[0].Backbuffer->GetDesc();
  ASSERT(backbufferDesc.Width == w);
  ASSERT(backbufferDesc.Height == h);
  ASSERT(backbufferDesc.Width <= UINT_MAX);
  m_SwapChainW = (unsigned int)backbufferDesc.Width;
  m_SwapChainH = backbufferDesc.Height;

#ifndef NO_ASSERTS
  for (UINT i = 0; i < k_FramesInFlight; ++i) {
    FrameData &fd = m_FrameData[i];
    D3D12_RESOURCE_DESC backbufferDescI = fd.Backbuffer->GetDesc();
    ASSERT(backbufferDescI.Width == m_SwapChainW);
    ASSERT(backbufferDescI.Height == m_SwapChainH);
  }
#endif // !NO_ASSERTS
}

void RenderingSystem::set_shader(Asset_Shader shader,
                                 ID3D12GraphicsCommandList10 *pCommandList) {
  if (m_CurrentShader.pPipelineState != shader.pPipelineState &&
      shader.pPipelineState) {
    pCommandList->SetPipelineState(shader.pPipelineState);
    pCommandList->SetGraphicsRootSignature(shader.pRootSignature);
  }

  if (m_CurrentShader.pRootSignature != shader.pRootSignature &&
      shader.pRootSignature) {
    pCommandList->SetGraphicsRootSignature(shader.pRootSignature);
  }

  m_CurrentShader = shader;
}

void RenderingSystem::wait_idle() {
  m_GPUStallValue++;
  m_GPUStallFence->SetEventOnCompletion(m_GPUStallValue, m_hGPUStallEvent);
  ASSERT_WIN_ALWAYS(
      m_CommandQueue->Signal(m_GPUStallFence.Get(), m_GPUStallValue));

  if (m_GPUStallFence->GetCompletedValue() < m_GPUStallValue) {
    WaitForSingleObject(m_hGPUStallEvent, INFINITE);
  }
}

ID3D12GraphicsCommandList10 *RenderingSystem::reset_staging_list() {
  ASSERT_WIN_ALWAYS(m_pStagingAllocator->Reset());
  ASSERT_WIN_ALWAYS(m_pStagingList->Reset(m_pStagingAllocator.Get(), NULL));

  return m_pStagingList.Get();
}

void RenderingSystem::execute_staging_list() {
  ASSERT_WIN_ALWAYS(m_pStagingList->Close());
  ID3D12CommandList *ppCommandLists[] = {m_pStagingList.Get()};
  m_CommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
  m_StagingFenceValue++;
  ASSERT_WIN_ALWAYS(
      m_CommandQueue->Signal(m_pStagingFence.Get(), m_StagingFenceValue));

  if (m_pStagingFence->GetCompletedValue() < m_StagingFenceValue) {
    m_pStagingFence->SetEventOnCompletion(m_StagingFenceValue,
                                          m_StagingFenceEvent);
    WaitForSingleObject(m_StagingFenceEvent, INFINITE);
  }
}

ID3D12Resource *RenderingSystem::upload_static_image_rgba(
    uint32_t w, uint32_t h, const void *data,
    D3D12_CPU_DESCRIPTOR_HANDLE *cpuHandle,
    D3D12_GPU_DESCRIPTOR_HANDLE *gpuHandle) {
  size_t size = (size_t)w * h * 4;

  ASSERT(m_DataHeapOffset + size <= m_DataHeapCapacity);
  ASSERT(m_DescriptorCount + 1 <= m_DescriptorCapacity);
  ASSERT(m_ResourceCount + 1 <= m_ResourceCapacity);

  D3D12_RESOURCE_DESC desc{
      .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
      .Alignment = 0,
      .Width = w,
      .Height = h,
      .DepthOrArraySize = 1,
      .MipLevels = 1,
      .Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
      .SampleDesc = {1, 0},
      .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
      .Flags = D3D12_RESOURCE_FLAG_NONE,
  };
  D3D12_RESOURCE_ALLOCATION_INFO info =
      m_pDevice->GetResourceAllocationInfo(0, 1, &desc);

  desc.Alignment = info.Alignment;

  size_t newOffset =
      (m_DataHeapOffset + info.Alignment - 1) & ~(info.Alignment - 1);

  ASSERT_ALWAYS(newOffset + info.SizeInBytes <= m_DataHeapCapacity);

  ASSERT_ALWAYS(m_StagingHeapSize >= size);

  // Flip Y
  {
    size_t pitch = (size_t)w * 4;
    for (uint32_t r = 0; r < h; ++r) {
      uint32_t src_row = h - r - 1;
      uint32_t dst_row = r;
      size_t src_offset = (size_t)src_row * pitch;
      size_t dst_offset = (size_t)dst_row * pitch;
      memcpy_s((char *)m_StagingHeapMap + dst_offset,
               m_StagingHeapSize - dst_offset, (char *)data + src_offset,
               pitch);
    }
  }

  ID3D12Resource *resource;
  ASSERT_WIN_ALWAYS(m_pDevice->CreatePlacedResource(
      m_DataHeap.Get(), newOffset, &desc, D3D12_RESOURCE_STATE_COPY_DEST,
      nullptr, IID_PPV_ARGS(&resource)));

  D3D12_CPU_DESCRIPTOR_HANDLE hDescriptor =
      m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
  hDescriptor.ptr += m_DescriptorCount * m_SrvCbvUabDescriptorIncrementSize;

  D3D12_SHADER_RESOURCE_VIEW_DESC resourceViewDesc{
      .Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
      .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
      .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
      .Texture2D =
          {
              .MostDetailedMip = 0,
              .MipLevels = (UINT)-1,
              .PlaneSlice = 0,
              .ResourceMinLODClamp = 0,
          },
  };
  m_pDevice->CreateShaderResourceView(resource, &resourceViewDesc, hDescriptor);

  ID3D12GraphicsCommandList10 *pCmdList = reset_staging_list();

  D3D12_TEXTURE_COPY_LOCATION dst{
      .pResource = resource,
      .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
      .SubresourceIndex = 0,
  };
  D3D12_TEXTURE_COPY_LOCATION src{
      .pResource = m_StagingResource.Get(),
      .Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
      .PlacedFootprint =
          {
              .Offset = 0,
              .Footprint =
                  {
                      .Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
                      .Width = w,
                      .Height = h,
                      .Depth = 1,
                      .RowPitch = w * 4,
                  },
          },
  };
  pCmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

  D3D12_RESOURCE_BARRIER barrier = {
      .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
      .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
      .Transition =
          {
              .pResource = resource,
              .Subresource = 0,
              .StateBefore = D3D12_RESOURCE_STATE_COPY_DEST,
              .StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
          },
  };
  pCmdList->ResourceBarrier(1, &barrier);

  execute_staging_list();

  *gpuHandle = m_DescriptorHeap->GetGPUDescriptorHandleForHeapStart();
  *cpuHandle = m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart();

  gpuHandle->ptr += m_DescriptorCount * m_SrvCbvUabDescriptorIncrementSize;
  cpuHandle->ptr += m_DescriptorCount * m_SrvCbvUabDescriptorIncrementSize;

  m_Resources[m_ResourceCount] = resource;

  m_DataHeapOffset = newOffset + info.SizeInBytes;
  ++m_DescriptorCount;
  ++m_ResourceCount;

  return resource;
}
