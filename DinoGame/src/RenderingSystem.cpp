#include "pch.h"

#include "RenderingSystem.h"

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

void RenderingSystem::init() {
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
  ASSERT_WIN_ALWAYS(CreateDXGIFactory1(IID_PPV_ARGS(&pDxgiFactory)));

  create_device(pDxgiFactory);

  ComPtr<ID3D12InfoQueue1> infoQueue;
  m_pDevice->QueryInterface(IID_PPV_ARGS(&infoQueue));

  DWORD callbackCookie = 0;
  ASSERT_WIN_ALWAYS(infoQueue->RegisterMessageCallback(
      d3d12_message_callback, D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr,
      &callbackCookie));
}

void RenderingSystem::stop() {
  ComPtr<ID3D12DebugDevice> debugDevice;
  ASSERT_WIN(m_pDevice->QueryInterface(IID_PPV_ARGS(&debugDevice)));
  ASSERT_WIN(debugDevice->ReportLiveDeviceObjects(
      D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL));
}

void RenderingSystem::create_device(ComPtr<IDXGIFactory6> pDxgiFactory) {
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

    if (!SUCCEEDED(D3D12CreateDevice(0, D3D_FEATURE_LEVEL_11_0,
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