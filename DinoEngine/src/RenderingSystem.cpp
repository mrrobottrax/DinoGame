#include "pch.h"

#include "RenderingSystem.h"
#include "WindowSystem.h"

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

void RenderingSystem::init() {
  ASSERT(g_WindowSystem.get_hWnd() != NULL);

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

#ifndef NDEBUG
  createFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif // NDEBUG

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
      &commandQueueDesc, IID_PPV_ARGS(&m_pCommandQueue)));

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
      m_pCommandQueue.Get(), g_WindowSystem.get_hWnd(), &swapChainDesc,
      &swapChainFullscreenDesc, NULL, &swapChain1));
  ASSERT_WIN_ALWAYS(swapChain1.As(&m_pSwapChain));

  DXGI_SWAP_CHAIN_DESC1 swapChainDescRetrieved;
  m_pSwapChain->GetDesc1(&swapChainDescRetrieved);
  m_SwapChainW = swapChainDescRetrieved.Width;
  m_SwapChainH = swapChainDescRetrieved.Height;

  ASSERT_WIN_ALWAYS(pDxgiFactory->MakeWindowAssociation(
      g_WindowSystem.get_hWnd(), DXGI_MWA_NO_ALT_ENTER));

  D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{
      .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
      .NumDescriptors = k_FramesInFlight,
      .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
      .NodeMask = 0,
  };
  ASSERT_WIN_ALWAYS(m_pDevice->CreateDescriptorHeap(
      &descriptorHeapDesc, IID_PPV_ARGS(&m_pFrameBufferDescriptorHeap)));

  m_RtvDescriptorIncrementSize = m_pDevice->GetDescriptorHandleIncrementSize(
      D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

  // create frame data
  for (UINT i = 0; i < k_FramesInFlight; ++i) {
    FrameData &fd = m_FrameData[i];
    ASSERT_WIN_ALWAYS(m_pDevice->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&fd.commandAllocator)));

    ASSERT_WIN_ALWAYS(m_pDevice->CreateCommandList1(
        0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE,
        IID_PPV_ARGS(&fd.commandList)));

    ASSERT_WIN_ALWAYS(m_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                                             IID_PPV_ARGS(&fd.fence)));

    fd.fenceEvent = CreateEventEx(NULL, NULL, 0, EVENT_ALL_ACCESS);
    ASSERT_ALWAYS(fd.fenceEvent != NULL);

    fd.fenceValue = 0;
  }

  create_backbuffer_data();

  m_Initialized = true;
}

void RenderingSystem::create_backbuffer_data() {
  for (UINT i = 0; i < k_FramesInFlight; ++i) {
    FrameData &fd = m_FrameData[i];

    ASSERT_WIN_ALWAYS(m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(&fd.backbuffer)));

    D3D12_RENDER_TARGET_VIEW_DESC rtViewDesc{
        .Format = k_SwapChainFormat,
        .ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
        .Texture2D =
            {
                .MipSlice = 0,
                .PlaneSlice = 0,
            },
    };
    D3D12_CPU_DESCRIPTOR_HANDLE handle =
        m_pFrameBufferDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += m_RtvDescriptorIncrementSize * i;
    m_pDevice->CreateRenderTargetView(fd.backbuffer.Get(), &rtViewDesc, handle);
  }
}

void RenderingSystem::wait_idle() {
  m_GPUStallValue++;
  m_GPUStallFence->SetEventOnCompletion(m_GPUStallValue, m_hGPUStallEvent);
  ASSERT_WIN_ALWAYS(
      m_pCommandQueue->Signal(m_GPUStallFence.Get(), m_GPUStallValue));

  if (m_GPUStallFence->GetCompletedValue() < m_GPUStallValue) {
    WaitForSingleObject(m_hGPUStallEvent, INFINITE);
  }
}

void RenderingSystem::stop() {
  m_Initialized = false;
  wait_idle();

  // destroy frame data
  for (UINT i = 0; i < k_FramesInFlight; ++i) {
    FrameData &fd = m_FrameData[i];
    CloseHandle(fd.fenceEvent);
    fd.fence.Reset();
    fd.backbuffer.Reset();
    fd.commandList.Reset();
    fd.commandAllocator.Reset();
  }

  m_pSwapChain.Reset();
  m_pCommandQueue.Reset();
  m_pFrameBufferDescriptorHeap.Reset();
  m_GPUStallFence.Reset();

  CloseHandle(m_hGPUStallEvent);

#if defined(_DEBUG)
  ComPtr<ID3D12DebugDevice> debugDevice;
  ASSERT_WIN(m_pDevice->QueryInterface(IID_PPV_ARGS(&debugDevice)));
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

void RenderingSystem::frame() {
  if (m_SwapChainW == 0 || m_SwapChainH == 0) {
    return;
  }

  UINT iFrame = m_pSwapChain->GetCurrentBackBufferIndex();

  FrameData &fd = m_FrameData[iFrame];
  if (fd.fence->GetCompletedValue() < fd.fenceValue) {
    fd.fence->SetEventOnCompletion(fd.fenceValue, fd.fenceEvent);
    WaitForSingleObject(fd.fenceEvent, INFINITE);
  }

  ASSERT(fd.commandAllocator);
  ASSERT(fd.commandList);

  ASSERT_WIN_ALWAYS(fd.commandAllocator->Reset());
  ASSERT_WIN_ALWAYS(fd.commandList->Reset(fd.commandAllocator.Get(), NULL));

  D3D12_RESOURCE_BARRIER unknownToRenderTargetBarrier{
      .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
      .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
      .Transition{
          .pResource = fd.backbuffer.Get(),
          .Subresource = 0,
          .StateBefore = D3D12_RESOURCE_STATE_COMMON,
          .StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET,
      }};
  fd.commandList->ResourceBarrier(1, &unknownToRenderTargetBarrier);

  D3D12_CPU_DESCRIPTOR_HANDLE rtvCpuHandle =
      m_pFrameBufferDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
  rtvCpuHandle.ptr += m_RtvDescriptorIncrementSize * iFrame;

  float color[4] = {0, 0, 0, 1};
  fd.commandList->ClearRenderTargetView(rtvCpuHandle, color, 0, NULL);

  fd.commandList->OMSetRenderTargets(1, &rtvCpuHandle, TRUE, nullptr);

  D3D12_RESOURCE_DESC1 backBufferDesc =
      m_FrameData[iFrame].backbuffer->GetDesc1();
  ASSERT(backBufferDesc.Width <= UINT_MAX);
  static unsigned int s_Count = 0;
  dgui_add_render_commands(fd.commandList.Get(),
                           (unsigned int)backBufferDesc.Width,
                           backBufferDesc.Height);

  D3D12_RESOURCE_BARRIER renderTargetToPresentBarrier{
      .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
      .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
      .Transition{
          .pResource = fd.backbuffer.Get(),
          .Subresource = 0,
          .StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET,
          .StateAfter = D3D12_RESOURCE_STATE_PRESENT,
      }};
  fd.commandList->ResourceBarrier(1, &renderTargetToPresentBarrier);

  ASSERT_WIN_ALWAYS(fd.commandList->Close());
  ID3D12CommandList *ppCommandLists[] = {fd.commandList.Get()};
  m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists),
                                       ppCommandLists);
  fd.fenceValue++;
  ASSERT_WIN_ALWAYS(m_pCommandQueue->Signal(fd.fence.Get(), fd.fenceValue));

  DXGI_PRESENT_PARAMETERS presentParameters{
      .DirtyRectsCount = 0,
      .pDirtyRects = nullptr,
      .pScrollRect = nullptr,
      .pScrollOffset = nullptr,
  };
  ASSERT_WIN_ALWAYS(m_pSwapChain->Present1(0, DXGI_PRESENT_ALLOW_TEARING,
                                           &presentParameters));
}

void RenderingSystem::try_resize(unsigned int w, unsigned int h) {
  if (!m_pSwapChain)
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
    fd.backbuffer.Reset();
  }

  ASSERT_WIN_ALWAYS(m_pSwapChain->ResizeBuffers(
      k_FramesInFlight, w, h, k_SwapChainFormat, k_SwapChainFlags));

  create_backbuffer_data();

  D3D12_RESOURCE_DESC1 backbufferDesc = m_FrameData[0].backbuffer->GetDesc1();
  ASSERT(backbufferDesc.Width == w);
  ASSERT(backbufferDesc.Height == h);
  ASSERT(backbufferDesc.Width <= UINT_MAX);
  m_SwapChainW = (unsigned int)backbufferDesc.Width;
  m_SwapChainH = backbufferDesc.Height;

#ifndef NO_ASSERTS
  for (UINT i = 0; i < k_FramesInFlight; ++i) {
    FrameData &fd = m_FrameData[i];
    D3D12_RESOURCE_DESC1 backbufferDescI = fd.backbuffer->GetDesc1();
    ASSERT(backbufferDescI.Width == m_SwapChainW);
    ASSERT(backbufferDescI.Height == m_SwapChainH);
  }
#endif // !NO_ASSERTS
}
