#include "pch.h"

#include "RenderingSystem.h"
#include "WindowSystem.h"

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
  ASSERT(g_WindowSystem.hWnd != NULL);

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
      .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
      .Stereo = FALSE,
      .SampleDesc =
          {
              .Count = 1,
              .Quality = 0,
          },
      .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
      .BufferCount = 3,
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
      m_pCommandQueue.Get(), g_WindowSystem.hWnd, &swapChainDesc,
      &swapChainFullscreenDesc, NULL, &swapChain1));
  ASSERT_WIN_ALWAYS(swapChain1.As(&m_pSwapChain));

  ASSERT_WIN_ALWAYS(pDxgiFactory->MakeWindowAssociation(g_WindowSystem.hWnd,
                                                        DXGI_MWA_NO_ALT_ENTER));
}

void RenderingSystem::stop() {
  m_pSwapChain.Reset();
  m_pCommandQueue.Reset();

  ComPtr<ID3D12DebugDevice> debugDevice;
  ASSERT_WIN(m_pDevice->QueryInterface(IID_PPV_ARGS(&debugDevice)));
  ASSERT_WIN(debugDevice->ReportLiveDeviceObjects(
      D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL));
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

    if (!SUCCEEDED(D3D12CreateDevice(pBestAdapter.Get(), D3D_FEATURE_LEVEL_11_0,
                                     IID_PPV_ARGS(&m_pDevice)))) {
      console_warn(
          "Adapter does not support D3D_FEATURE_LEVEL_11_0. Trying again:");
      pBestAdapter.Reset();

      // deliberately don't reset i.
    } else {
      break;
    }
  }
}