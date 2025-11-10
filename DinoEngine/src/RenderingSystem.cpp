#include "pch.h"

#include "AssetSystem.h"
#include "GameDllSystem.h"
#include "RenderingSystem.h"
#include "UISystem.h"
#include "WindowSystem.h"
#include "asset_types.h"

DINO_API IRenderingSystem *g_IRenderingSystem = &g_RenderingSystem;

constexpr UINT k_SwapChainFlags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
constexpr DXGI_FORMAT k_SwapChainFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

static Asset_Shader s_SRGBShader;

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

  // enable the D3D12 debug layer
#if defined(_DEBUG)
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
#endif //  defined(_DEBUG)

  // create DXGI factory
  ComPtr<IDXGIFactory6> pDxgiFactory;
  {
    UINT createFactoryFlags = 0;

#if defined(_DEBUG)
    createFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

    ASSERT_WIN_ALWAYS(
        CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&pDxgiFactory)));

    BOOL allowsTearing = FALSE;
    ASSERT_WIN_ALWAYS(pDxgiFactory->CheckFeatureSupport(
        DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowsTearing,
        sizeof(allowsTearing)));
    ASSERT_ALWAYS(allowsTearing);
  }

  // create device
  {
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

      if (!SUCCEEDED(D3D12CreateDevice(pBestAdapter.Get(),
                                       D3D_FEATURE_LEVEL_12_1,
                                       IID_PPV_ARGS(&m_Device)))) {
        console_warn(
            "Adapter does not support D3D_FEATURE_LEVEL_12_1. Trying again:");
        pBestAdapter.Reset();

        // deliberately don't reset i.
      } else {
        break;
      }
    }

#if defined(_DEBUG)

    // custom message callback
    ComPtr<ID3D12InfoQueue1> infoQueue;
    ASSERT_WIN_ALWAYS(m_Device->QueryInterface(IID_PPV_ARGS(&infoQueue)));

#pragma warning(push)
#pragma warning(disable : 6387)
    DWORD callbackCookie = 0;
    ASSERT_WIN_ALWAYS(infoQueue->RegisterMessageCallback(
        d3d12_message_callback, D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr,
        &callbackCookie));
#pragma warning(pop)

#endif
  }

  // get increment sizes
  m_RtvDescriptorIncrementSize = m_Device->GetDescriptorHandleIncrementSize(
      D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  m_SrvCbvUabDescriptorIncrementSize =
      m_Device->GetDescriptorHandleIncrementSize(
          D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

  // create stall fence / event
  m_hGPUStallEvent = CreateEventEx(NULL, L"Big GPU stall", 0, EVENT_ALL_ACCESS);
  ASSERT_WIN_EXP_ALWAYS(m_hGPUStallEvent != NULL);

  ASSERT_WIN_ALWAYS(m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                                          IID_PPV_ARGS(&m_GPUStallFence)));

  // create command queue
  D3D12_COMMAND_QUEUE_DESC commandQueueDesc{
      .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
      .Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH,
      .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
      .NodeMask = 0,
  };
  ASSERT_WIN_ALWAYS(m_Device->CreateCommandQueue(
      &commandQueueDesc, IID_PPV_ARGS(&m_CommandQueue)));

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
    ASSERT_WIN_ALWAYS(swapChain1.As(&m_SwapChain.SwapChain));

    DXGI_SWAP_CHAIN_DESC1 swapChainDescRetrieved;
    m_SwapChain.SwapChain->GetDesc1(&swapChainDescRetrieved);
    m_SwapChain.Width = swapChainDescRetrieved.Width;
    m_SwapChain.Height = swapChainDescRetrieved.Height;

    ASSERT_WIN_ALWAYS(pDxgiFactory->MakeWindowAssociation(
        g_WindowSystem.get_hWnd(), DXGI_MWA_NO_ALT_ENTER));

    // create backbuffer rtv descriptor heap
    {
      D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{
          .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
          .NumDescriptors = k_FramesInFlight,
          .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
          .NodeMask = 0,
      };
      ASSERT_WIN_ALWAYS(m_Device->CreateDescriptorHeap(
          &descriptorHeapDesc,
          IID_PPV_ARGS(&m_SwapChain.BackBuffer_RTVDescriptorHeap)));
    }

    // create render texture srvHandle descriptor heap
    {
      D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{
          .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
          .NumDescriptors = k_FramesInFlight * 2,
          .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
          .NodeMask = 0,
      };
      ASSERT_WIN_ALWAYS(m_Device->CreateDescriptorHeap(
          &descriptorHeapDesc,
          IID_PPV_ARGS(&m_SwapChain.RenderTexture_SRVDescriptorHeap)));
    }

    // create render texture rtv descriptor heap
    {
      D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{
          .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
          .NumDescriptors = k_FramesInFlight * 2,
          .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
          .NodeMask = 0,
      };
      ASSERT_WIN_ALWAYS(m_Device->CreateDescriptorHeap(
          &descriptorHeapDesc,
          IID_PPV_ARGS(&m_SwapChain.RenderTexture_RTVDescriptorHeap)));
    }
  }

  // create staging data
  {
    ASSERT_WIN_ALWAYS(m_Device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_pStagingAllocator)));

    ASSERT_WIN_ALWAYS(m_Device->CreateCommandList1(
        0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE,
        IID_PPV_ARGS(&m_pStagingList)));

    ASSERT_WIN_ALWAYS(m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
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
        m_Device->CreateHeap(&stagingHeapDesc, IID_PPV_ARGS(&m_StagingHeap)));

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
    ASSERT_WIN_ALWAYS(m_Device->CreatePlacedResource(
        m_StagingHeap.Get(), 0, &stagingBufferDesc,
        D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr,
        IID_PPV_ARGS(&m_StagingResource)));

    ASSERT_WIN_ALWAYS(m_StagingResource->Map(0, nullptr, &m_StagingHeapMap));
  }

  // create frame data
  {
    for (UINT i = 0; i < k_FramesInFlight; ++i) {
      FrameData &fd = m_SwapChain.FrameData[i];
      ASSERT_WIN_ALWAYS(m_Device->CreateCommandAllocator(
          D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&fd.CommandAllocator)));

      ASSERT_WIN_ALWAYS(m_Device->CreateCommandList1(
          0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE,
          IID_PPV_ARGS(&fd.CommandList)));

      ASSERT_WIN_ALWAYS(m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
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
    ASSERT_WIN_ALWAYS(m_Device->CreateDescriptorHeap(
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
        m_Device->CreateHeap(&staticHeapDesc, IID_PPV_ARGS(&m_DataHeap)));
  }

  // create resources array
  {
    m_ResourceCapacity = game.GPUMaxResources;
    m_Resources = (ID3D12Resource **)malloc(m_ResourceCapacity *
                                            sizeof(ID3D12Resource *));
    ASSERT_ALWAYS(m_Resources);
  }

  s_SRGBShader =
      compile_compute_shader("shaders\\DinoEngine\\LinearToSrgb.cso");

  m_IsInitialized = true;
}

void RenderingSystem::stop() {
  m_IsInitialized = false;
  wait_idle();

  // release shaders
  s_SRGBShader.release();

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
    FrameData &fd = m_SwapChain.FrameData[i];
    CloseHandle(fd.FenceEvent);
    fd.Fence.Reset();
    fd.Backbuffer.Reset();
    fd.RenderTextures[0].Reset();
    fd.RenderTextures[1].Reset();
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

  // destroy swapchain and render targets
  m_SwapChain.RenderTexture_RTVDescriptorHeap.Reset();
  m_SwapChain.RenderTexture_SRVDescriptorHeap.Reset();
  m_SwapChain.BackBuffer_RTVDescriptorHeap.Reset();
  m_SwapChain.SwapChain.Reset();

  // destroy others
  m_CommandQueue.Reset();
  m_GPUStallFence.Reset();

  CloseHandle(m_hGPUStallEvent);

#if defined(_DEBUG)
  ComPtr<ID3D12DebugDevice> debugDevice;
  ASSERT_WIN(m_Device->QueryInterface(IID_PPV_ARGS(&debugDevice)));
#endif
  m_Device.Reset();
#if defined(_DEBUG)
  ASSERT_WIN(debugDevice->ReportLiveDeviceObjects(
      D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL));
#endif
}

void RenderingSystem::create_backbuffer_data() {
  // create backbuffer rtvs
  for (UINT i = 0; i < k_FramesInFlight; ++i) {
    FrameData &fd = m_SwapChain.FrameData[i];

    ASSERT_WIN_ALWAYS(
        m_SwapChain.SwapChain->GetBuffer(i, IID_PPV_ARGS(&fd.Backbuffer)));

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle =
        m_SwapChain.BackBuffer_RTVDescriptorHeap
            ->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += m_RtvDescriptorIncrementSize * i;

    m_Device->CreateRenderTargetView(fd.Backbuffer.Get(), nullptr, rtvHandle);
  }

  D3D12_RESOURCE_DESC swDesc = m_SwapChain.FrameData[0].Backbuffer->GetDesc();

  // create render textures, rtvs, and srvs
  for (UINT i = 0; i < k_FramesInFlight; ++i) {
    FrameData &fd = m_SwapChain.FrameData[i];

    fd.RenderTextures[0].Reset();
    fd.RenderTextures[1].Reset();

    // create resources
    D3D12_CLEAR_VALUE clear{
        .Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
        .Color = {0, 0, 0, 1},
    };
    D3D12_HEAP_PROPERTIES props{
        .Type = D3D12_HEAP_TYPE_DEFAULT,
    };
    D3D12_RESOURCE_DESC desc{
        .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        .Alignment = 0,
        .Width = swDesc.Width,
        .Height = swDesc.Height,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
        .SampleDesc = {1, 0},
        .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
        .Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
    };

    ASSERT_WIN_ALWAYS(m_Device->CreateCommittedResource(
        &props, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_RENDER_TARGET,
        &clear, IID_PPV_ARGS(&fd.RenderTextures[0])));

    ASSERT_WIN_ALWAYS(m_Device->CreateCommittedResource(
        &props, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_RENDER_TARGET,
        &clear, IID_PPV_ARGS(&fd.RenderTextures[1])));

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle =
        m_SwapChain.RenderTexture_RTVDescriptorHeap
            ->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += m_RtvDescriptorIncrementSize * i * 2;

    D3D12_CPU_DESCRIPTOR_HANDLE srvHandle =
        m_SwapChain.RenderTexture_SRVDescriptorHeap
            ->GetCPUDescriptorHandleForHeapStart();
    srvHandle.ptr += m_SrvCbvUabDescriptorIncrementSize * i * 2;

    // create rtv
    m_Device->CreateRenderTargetView(fd.RenderTextures[0].Get(), nullptr,
                                     rtvHandle);
    rtvHandle.ptr += m_RtvDescriptorIncrementSize;
    m_Device->CreateRenderTargetView(fd.RenderTextures[1].Get(), nullptr,
                                     rtvHandle);

    // create srv
    m_Device->CreateShaderResourceView(fd.RenderTextures[0].Get(), nullptr,
                                       srvHandle);
    srvHandle.ptr += m_SrvCbvUabDescriptorIncrementSize;
    m_Device->CreateShaderResourceView(fd.RenderTextures[1].Get(), nullptr,
                                       srvHandle);
  }
}

void RenderingSystem::frame() {
  if (m_SwapChain.Width == 0 || m_SwapChain.Height == 0) {
    return;
  }

  // SETUP
  UINT iFrame = m_SwapChain.SwapChain->GetCurrentBackBufferIndex();

  FrameData &fd = m_SwapChain.FrameData[iFrame];
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
      m_SwapChain.BackBuffer_RTVDescriptorHeap
          ->GetCPUDescriptorHandleForHeapStart();
  rtvCpuHandle.ptr += m_RtvDescriptorIncrementSize * iFrame;

  float color[4] = {0, 0, 0, 1};
  fd.CommandList->ClearRenderTargetView(rtvCpuHandle, color, 0, NULL);

  fd.CommandList->OMSetRenderTargets(1, &rtvCpuHandle, TRUE, nullptr);

  D3D12_RESOURCE_DESC backBufferDesc =
      m_SwapChain.FrameData[iFrame].Backbuffer->GetDesc();
  ASSERT(backBufferDesc.Width <= UINT_MAX);

  // RENDER UI
  g_UISystem.add_render_commands(fd.CommandList.Get(),
                                 (uint32_t)backBufferDesc.Width,
                                 backBufferDesc.Height);

  // CONVERT TO SRGB
  /*{
    fd.CommandList->SetPipelineState(s_SRGBShader.PipelineState);
    fd.CommandList->SetComputeRootSignature(s_SRGBShader.RootSignature);

    uint32_t constants[] = {
        (uint32_t)backBufferDesc.Width,
        (uint32_t)backBufferDesc.Height,
    };
    fd.CommandList->SetComputeRoot32BitConstants(0, 2, constants, 0);

    constexpr UINT k_ThreadCount = 16;
    UINT x = (UINT)((backBufferDesc.Width + k_ThreadCount - 1) / k_ThreadCount);
    UINT y =
        (UINT)((backBufferDesc.Height + k_ThreadCount - 1) / k_ThreadCount);
    fd.CommandList->Dispatch(x, y, 1);
  }*/

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
  ASSERT_WIN_ALWAYS(m_SwapChain.SwapChain->Present1(
      0, DXGI_PRESENT_ALLOW_TEARING, &presentParameters));
}

void RenderingSystem::try_resize(unsigned int w, unsigned int h) {
  if (!m_SwapChain.SwapChain)
    return;

  if (m_SwapChain.Width == w && m_SwapChain.Height == h)
    return;

  console_log("SIZE: %u, %u", w, h);
  if (w == 0 || h == 0) {
    m_SwapChain.Width = m_SwapChain.Height = 0;
    return;
  }

  wait_idle();

  for (UINT i = 0; i < k_FramesInFlight; ++i) {
    FrameData &fd = m_SwapChain.FrameData[i];
    fd.Backbuffer.Reset();
  }

  ASSERT_WIN_ALWAYS(m_SwapChain.SwapChain->ResizeBuffers(
      k_FramesInFlight, w, h, k_SwapChainFormat, k_SwapChainFlags));

  create_backbuffer_data();

  D3D12_RESOURCE_DESC backbufferDesc =
      m_SwapChain.FrameData[0].Backbuffer->GetDesc();
  ASSERT(backbufferDesc.Width == w);
  ASSERT(backbufferDesc.Height == h);
  ASSERT(backbufferDesc.Width <= UINT_MAX);
  m_SwapChain.Width = (unsigned int)backbufferDesc.Width;
  m_SwapChain.Height = backbufferDesc.Height;

#ifndef NO_ASSERTS
  for (UINT i = 0; i < k_FramesInFlight; ++i) {
    FrameData &fd = m_SwapChain.FrameData[i];
    D3D12_RESOURCE_DESC backbufferDescI = fd.Backbuffer->GetDesc();
    ASSERT(backbufferDescI.Width == m_SwapChain.Width);
    ASSERT(backbufferDescI.Height == m_SwapChain.Height);
  }
#endif // !NO_ASSERTS
}

void RenderingSystem::set_shader(Asset_Shader shader,
                                 ID3D12GraphicsCommandList10 *pCommandList) {
  if (m_CurrentShader.PipelineState != shader.PipelineState &&
      shader.PipelineState) {
    pCommandList->SetPipelineState(shader.PipelineState);
    pCommandList->SetGraphicsRootSignature(shader.RootSignature);
  }

  if (m_CurrentShader.RootSignature != shader.RootSignature &&
      shader.RootSignature) {
    pCommandList->SetGraphicsRootSignature(shader.RootSignature);
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
      m_Device->GetResourceAllocationInfo(0, 1, &desc);

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
  ASSERT_WIN_ALWAYS(m_Device->CreatePlacedResource(
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
  m_Device->CreateShaderResourceView(resource, &resourceViewDesc, hDescriptor);

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

Asset_Shader RenderingSystem::compile_transparent_quad_shader(
    const char *vertPath, const char *fragPath,
    ID3D12RootSignature *pRootSignature) const {
  ID3D12Device9 *device = g_RenderingSystem.get_device();
  ASSERT(device);

  ResourceLoader_arena0_reset();
  ResourceLoader_arena1_reset();

  void *vsFile;
  size_t vsSize;
  ASSERT_CODE_ALWAYS(ResourceLoader_load_file(vertPath, &vsFile, &vsSize,
                                              ResourceLoader_arena0));

  void *fsFile;
  size_t fsSize;
  ASSERT_CODE_ALWAYS(ResourceLoader_load_file(fragPath, &fsFile, &fsSize,
                                              ResourceLoader_arena1));

  if (!pRootSignature) {
    ComPtr<ID3DBlob> pVSRootSignatureBlob;
    ASSERT_WIN_ALWAYS(D3DGetBlobPart(vsFile, vsSize, D3D_BLOB_ROOT_SIGNATURE, 0,
                                     &pVSRootSignatureBlob));

    ComPtr<ID3DBlob> pPSRootSignatureBlob;
    ASSERT_WIN_ALWAYS(D3DGetBlobPart(fsFile, fsSize, D3D_BLOB_ROOT_SIGNATURE, 0,
                                     &pPSRootSignatureBlob));

    bool rootSignaturesEqual = false;
    if (pVSRootSignatureBlob.Get() && pPSRootSignatureBlob.Get() &&
        pVSRootSignatureBlob->GetBufferSize() ==
            pPSRootSignatureBlob->GetBufferSize()) {
      rootSignaturesEqual =
          (memcmp(pVSRootSignatureBlob->GetBufferPointer(),
                  pPSRootSignatureBlob->GetBufferPointer(),
                  pVSRootSignatureBlob->GetBufferSize()) == 0);
    }

    ASSERT_WIN_ALWAYS(device->CreateRootSignature(
        0, pVSRootSignatureBlob->GetBufferPointer(),
        pVSRootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&pRootSignature)));
  }

  D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{
      .pRootSignature = pRootSignature,
      .VS =
          {
              .pShaderBytecode = vsFile,
              .BytecodeLength = vsSize,
          },
      .PS =
          {
              .pShaderBytecode = fsFile,
              .BytecodeLength = fsSize,
          },
      .DS = {},
      .HS = {},
      .GS = {},
      .StreamOutput =
          {
              .pSODeclaration = nullptr,
              .NumEntries = 0,
              .pBufferStrides = nullptr,
              .NumStrides = 0,
              .RasterizedStream = 0,
          },
      .BlendState =
          {
              .AlphaToCoverageEnable = FALSE,
              .IndependentBlendEnable = FALSE,
              .RenderTarget = {{
                  .BlendEnable = TRUE,
                  .LogicOpEnable = FALSE,
                  .SrcBlend = D3D12_BLEND_SRC_ALPHA,
                  .DestBlend = D3D12_BLEND_INV_SRC_ALPHA,
                  .BlendOp = D3D12_BLEND_OP_ADD,
                  .SrcBlendAlpha = D3D12_BLEND_ONE,
                  .DestBlendAlpha = D3D12_BLEND_ZERO,
                  .BlendOpAlpha = D3D12_BLEND_OP_ADD,
                  .LogicOp = D3D12_LOGIC_OP_NOOP,
                  .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL,
              }},
          },
      .SampleMask = 0xFFFFFFFF,
      .RasterizerState =
          {
              .FillMode = D3D12_FILL_MODE_SOLID,
              .CullMode = D3D12_CULL_MODE_BACK,
              .FrontCounterClockwise = TRUE,
              .DepthBias = 0,
              .DepthBiasClamp = 0,
              .SlopeScaledDepthBias = 0,
              .DepthClipEnable = TRUE,
              .MultisampleEnable = FALSE,
              .AntialiasedLineEnable = FALSE,
              .ForcedSampleCount = 0,
              .ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,
          },
      .DepthStencilState =
          {
              .DepthEnable = FALSE,
              .DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO,
              .DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS,
              .StencilEnable = FALSE,
              .StencilReadMask = 0,
              .StencilWriteMask = 0,
              .FrontFace = D3D12_STENCIL_OP_KEEP,
              .BackFace = D3D12_STENCIL_OP_KEEP,
          },
      .InputLayout =
          {
              .pInputElementDescs = nullptr,
              .NumElements = 0,
          },
      .IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,
      .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
      .NumRenderTargets = 1,
      .RTVFormats = {DXGI_FORMAT_R8G8B8A8_UNORM},
      .DSVFormat = DXGI_FORMAT_UNKNOWN,
      .SampleDesc =
          {
              .Count = 1,
              .Quality = 0,
          },
      .NodeMask = 0,
      .CachedPSO = {.pCachedBlob = nullptr, .CachedBlobSizeInBytes = 0},
      .Flags = D3D12_PIPELINE_STATE_FLAG_NONE,
  };

  ID3D12PipelineState *pipelineState;
  ASSERT_WIN_ALWAYS(device->CreateGraphicsPipelineState(
      &graphicsPipelineStateDesc, IID_PPV_ARGS(&pipelineState)));

  ResourceLoader_arena0_reset();
  ResourceLoader_arena1_reset();

  return {
      .PipelineState = pipelineState,
      .RootSignature = pRootSignature,
  };
}

Asset_Shader RenderingSystem::compile_compute_shader(
    const char *path, ID3D12RootSignature *pRootSignature) const {
  ASSERT(m_Device);

  ResourceLoader_arena0_reset();
  ResourceLoader_arena1_reset();

  void *file;
  size_t size;
  ASSERT_CODE_ALWAYS(
      ResourceLoader_load_file(path, &file, &size, ResourceLoader_arena0));

  if (!pRootSignature) {
    ComPtr<ID3DBlob> pRootSignatureBlob;
    ASSERT_WIN_ALWAYS(D3DGetBlobPart(file, size, D3D_BLOB_ROOT_SIGNATURE, 0,
                                     &pRootSignatureBlob));

    ASSERT_WIN_ALWAYS(m_Device->CreateRootSignature(
        0, pRootSignatureBlob->GetBufferPointer(),
        pRootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&pRootSignature)));
  }

  D3D12_COMPUTE_PIPELINE_STATE_DESC computePipelineStateDesc{
      .pRootSignature = pRootSignature,
      .CS =
          {
              .pShaderBytecode = file,
              .BytecodeLength = size,
          },
      .NodeMask = 0,
      .CachedPSO = nullptr,
      .Flags = D3D12_PIPELINE_STATE_FLAG_NONE,
  };

  ID3D12PipelineState *pipelineState;
  ASSERT_WIN_ALWAYS(m_Device->CreateComputePipelineState(
      &computePipelineStateDesc, IID_PPV_ARGS(&pipelineState)));

  ResourceLoader_arena0_reset();
  ResourceLoader_arena1_reset();

  return {
      .PipelineState = pipelineState,
      .RootSignature = pRootSignature,
  };
}