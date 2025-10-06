#include "pch.h"

#include "AssetSystem.h"
#include "GameDllSystem.h"
#include "RenderingSystem.h"

constexpr size_t k_DecompressionBufferSize = 1 << 20; // 1MB

DINO_API IAssetSystem *get_asset_system_interface() {
  return (IAssetSystem *)&g_AssetSystem;
}

void AssetSystem::init() {
  ASSERT_ALWAYS(g_RenderingSystem.is_initialized());

  m_LevelHeapCapacity = g_GameDllSystem.GameInfo.StaticLevelHeapSize;
  m_LevelResourceCapacity =
      g_GameDllSystem.GameInfo.StaticLevelResourceCapacity;
  m_StagingBufferCapacity = g_GameDllSystem.GameInfo.StagingBufferCapacity;

  ID3D12Device9 *pDevice = g_RenderingSystem.get_device();

  // Create level heap
  {
    D3D12_HEAP_DESC levelHeapDesc{
        .SizeInBytes = m_LevelHeapCapacity,
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
    ASSERT_WIN_ALWAYS(
        pDevice->CreateHeap(&levelHeapDesc, IID_PPV_ARGS(&m_LevelHeap)));

    D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{
        .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        .NumDescriptors = (UINT)m_LevelResourceCapacity,
        .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        .NodeMask = 0,
    };
    ASSERT_WIN_ALWAYS(pDevice->CreateDescriptorHeap(
        &descriptorHeapDesc, IID_PPV_ARGS(&m_LevelDescriptorHeap)));

    m_LevelResources = (ID3D12Resource2 **)malloc(sizeof(ID3D12Resource2 *) *
                                                  m_LevelResourceCapacity);
    ASSERT_ALWAYS(m_LevelResources);
  }

  // Create staging buffer
  {
    D3D12_HEAP_PROPERTIES uploadHeap{
        .Type = D3D12_HEAP_TYPE_UPLOAD,
        .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
        .CreationNodeMask = 0,
        .VisibleNodeMask = 0,
    };
    D3D12_RESOURCE_DESC uploadHeapDesc{
        .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
        .Alignment = 0,
        .Width = m_StagingBufferCapacity,
        .Height = 1,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = DXGI_FORMAT_UNKNOWN,
        .SampleDesc = {1, 0},
        .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        .Flags = D3D12_RESOURCE_FLAG_NONE,
    };

    D3D12_RESOURCE_ALLOCATION_INFO info =
        pDevice->GetResourceAllocationInfo(0, 1, &uploadHeapDesc);

    uploadHeapDesc.Alignment = info.Alignment;

    ASSERT_WIN_ALWAYS(pDevice->CreateCommittedResource(
        &uploadHeap, D3D12_HEAP_FLAG_NONE, &uploadHeapDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&m_StagingBuffer)));

    D3D12_RANGE range{0, 0};
    m_StagingBuffer->Map(0, &range, (void **)&m_StagingBufferMap);
  }
}

void AssetSystem::stop() {
  wipe_level_assets();

  m_StagingBuffer->Unmap(0, nullptr);
  m_StagingBuffer.Reset();

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
  ASSERT_WIN_CODE_ALWAYS(ResourceLoader_load_file(path),
                         "Failed to load file: %s", path);

  PngOutInfo png;
  ASSERT_WIN_CODE_ALWAYS(ResourceLoader_decompress_png(&png),
                         "Failed to decompress png.");

  ID3D12Device9 *pDevice = g_RenderingSystem.get_device();

  // Check allocation info
  D3D12_RESOURCE_DESC desc{
      .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
      .Alignment = 0,
      .Width = png.Width,
      .Height = png.Height,
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

  if (m_LevelHeapOffset > m_LevelHeapCapacity) {
    CRASH("Out of room in level data heap!");
  }

  if (m_LevelDescriptorCount + 1 > m_LevelResourceCapacity) {
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
      m_LevelHeap.Get(), alignedOffset, &desc, D3D12_RESOURCE_STATE_COPY_DEST,
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

  UINT pitch = ((png.Width * 4 + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1) /
                D3D12_TEXTURE_DATA_PITCH_ALIGNMENT) *
               D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;

  if ((size_t)pitch * png.Height > m_StagingBufferCapacity) {
    CRASH("Staging buffer too small!");
  }

  // Copy into staging buffer
  for (size_t y = 0; y < png.Height; ++y) {
    for (size_t x = 0; x < png.Width; ++x) {
      size_t i = y * pitch + x * 4;
      size_t j = y * png.Width * 4 + x * 4;
      m_StagingBufferMap[i + 0] = png.Data[j + 0];
      m_StagingBufferMap[i + 1] = png.Data[j + 1];
      m_StagingBufferMap[i + 2] = png.Data[j + 2];
      m_StagingBufferMap[i + 3] = png.Data[j + 3];
    }
  }

  // Copy from staging buffer
  ID3D12GraphicsCommandList10 *pList = g_RenderingSystem.record_staging_list();

  D3D12_TEXTURE_COPY_LOCATION dst{
      .pResource = m_LevelResources[m_LevelResourceCount],
      .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
      .SubresourceIndex = 0,
  };
  D3D12_TEXTURE_COPY_LOCATION src{
      .pResource = m_StagingBuffer.Get(),
      .Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
      .PlacedFootprint = {
          .Offset = 0,
          .Footprint =
              {
                  .Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
                  .Width = png.Width,
                  .Height = png.Height,
                  .Depth = 1,
                  .RowPitch = pitch,
              },
      }};
  pList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

  D3D12_RESOURCE_BARRIER barrier{
      .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
      .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
      .Transition = {
          .pResource = m_LevelResources[m_LevelResourceCount],
          .Subresource = 0,
          .StateBefore = D3D12_RESOURCE_STATE_COPY_DEST,
          .StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
                        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
      }};
  pList->ResourceBarrier(1, &barrier);

  g_RenderingSystem.execute_staging_list();

  ++m_LevelDescriptorCount;
  ++m_LevelResourceCount;

  return GPUImage(gpuHandle, m_LevelDescriptorHeap.Get());
}