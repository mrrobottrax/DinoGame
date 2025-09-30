#include "pch.h"

#include "AssetSystem.h"
#include "RenderingSystem.h"

constexpr size_t k_LevelHeapSize = 1 << 20;        // 1 MB
constexpr size_t k_LevelDescriptorCount = 1 << 10; // 1024
constexpr size_t k_LevelResourceCount = 1 << 10;   // 1024

DINO_API IAssetSystem *get_asset_system_interface() {
  return (IAssetSystem *)&g_AssetSystem;
}

void AssetSystem::init() {
  ASSERT_ALWAYS(g_RenderingSystem.is_initialized());

  D3D12_HEAP_DESC levelHeapDesc{
      .SizeInBytes = k_LevelHeapSize,
      .Properties =
          {
              .Type = D3D12_HEAP_TYPE_DEFAULT,
              .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
              .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
              .CreationNodeMask = 0,
              .VisibleNodeMask = 0,
          },
      .Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
      .Flags = D3D12_HEAP_FLAG_NONE,
  };
  ID3D12Device9 *pDevice = g_RenderingSystem.get_device();
  ASSERT_WIN_ALWAYS(
      pDevice->CreateHeap(&levelHeapDesc, IID_PPV_ARGS(&m_LevelHeap)));

  D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{
      .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
      .NumDescriptors = k_LevelDescriptorCount,
      .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
      .NodeMask = 0,
  };
  pDevice->CreateDescriptorHeap(&descriptorHeapDesc,
                                IID_PPV_ARGS(&m_LevelDescriptorHeap));

  m_LevelResources = (ID3D12Resource2 **)malloc(sizeof(ID3D12Resource2 *) *
                                                k_LevelResourceCount);
}

void AssetSystem::stop() {
  wipe_level_assets();

  free(m_LevelResources);
  m_LevelDescriptorHeap.Reset();
  m_LevelHeap.Reset();
}

void AssetSystem::wipe_level_assets() {
  m_LevelHeapOffset = 0;
  m_LevelDescriptorCount = 0;

  for (size_t i = 0; i < m_LevelResourceCount; ++i) {
    m_LevelResources[i]->Release();
  }

  m_LevelResourceCount = 0;
}

GPUImage AssetSystem::load_png(const char *path) {
  ID3D12Device9 *pDevice = g_RenderingSystem.get_device();

  // Check allocation info
  D3D12_RESOURCE_DESC desc{
      .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
      .Alignment = 0,
      .Width = 512,
      .Height = 512,
      .DepthOrArraySize = 1,
      .MipLevels = 1,
      .Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
      .SampleDesc = {1, 0},
      .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
      .Flags = D3D12_RESOURCE_FLAG_NONE,
  };

  D3D12_RESOURCE_ALLOCATION_INFO info =
      pDevice->GetResourceAllocationInfo(0, 1, &desc);

  desc.Alignment = info.Alignment;

  // Check if has room
  size_t alignedOffset =
      ((m_LevelHeapOffset + info.Alignment - 1) / info.Alignment) *
      info.Alignment;
  m_LevelHeapOffset = alignedOffset + info.SizeInBytes;

  if (m_LevelHeapOffset > k_LevelHeapSize) {
    CRASH("Out of room in level data heap!");
  }

  if (m_LevelDescriptorCount + 1 > k_LevelDescriptorCount) {
    CRASH("Out of level descriptors!");
  }

  // Get descriptor handles
  D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle =
      m_LevelDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
  cpuHandle.ptr +=
      m_LevelDescriptorCount * pDevice->GetDescriptorHandleIncrementSize(
                                   D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

  D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle =
      m_LevelDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
  gpuHandle.ptr +=
      m_LevelDescriptorCount * pDevice->GetDescriptorHandleIncrementSize(
                                   D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

  // Create resource
  ASSERT_WIN_ALWAYS(pDevice->CreatePlacedResource(
      m_LevelHeap.Get(), alignedOffset, &desc,
      D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
          D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
      nullptr, IID_PPV_ARGS(&m_LevelResources[m_LevelResourceCount])));

  // Create view
  D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc{
      .Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
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
  pDevice->CreateShaderResourceView(m_LevelResources[m_LevelResourceCount],
                                    &viewDesc, cpuHandle);

  ++m_LevelDescriptorCount;
  ++m_LevelResourceCount;

  return GPUImage(gpuHandle, m_LevelDescriptorHeap.Get());
}