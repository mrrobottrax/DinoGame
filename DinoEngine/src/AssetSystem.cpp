#include "pch.h"

#include "AssetSystem.h"
#include "GameDllSystem.h"
#include "RenderingSystem.h"

DINO_API IAssetSystem *g_IAssetSystem = (IAssetSystem *)&g_AssetSystem;

void AssetSystem::start() {
  ASSERT_ALWAYS(g_RenderingSystem.is_initialized());

  m_LevelHeapCapacity = g_GameDllSystem.GameInfo.StaticLevelHeapSize;
  m_LevelResourceCapacity =
      g_GameDllSystem.GameInfo.StaticLevelResourceCapacity;
  m_StagingBufferCapacity = g_GameDllSystem.GameInfo.StagingBufferCapacity;
  m_ShaderCapacity = g_GameDllSystem.GameInfo.ShaderCapacity;

  m_pShaders =
      (ShaderContainer *)calloc(1, sizeof(ShaderContainer) * m_ShaderCapacity);
  ASSERT_ALWAYS(m_pShaders);
  m_pFirstEmptyShader = m_pShaders;
  m_pFirstEmptyShader->BlockSize = sizeof(ShaderContainer) * m_ShaderCapacity;

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

    m_Initialized = true;
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

  m_Initialized = false;

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

HAsset_Shader AssetSystem::load_shader(const char *vertexPath,
                                       const char *fragmentPath,
                                       const char *assetName,
                                       ID3D12RootSignature *pRootSignature,
                                       EAssetScope scope) {
  ASSERT_ALWAYS(g_RenderingSystem.is_initialized());
  ASSERT_ALWAYS(is_initialized());
  ID3D12Device9 *pDevice = g_RenderingSystem.get_device();

  ASSERT_ALWAYS(m_pFirstEmptyShader, "Shader capacity exceeded");
  ASSERT_ALWAYS(m_pFirstEmptyShader->BlockSize >= sizeof(ShaderContainer),
                "Shader capacity exceeded");

  void *pVertexFile;
  size_t vertexFileSize;
  ASSERT_CODE_ALWAYS(ResourceLoader_load_file(
      vertexPath, &pVertexFile, &vertexFileSize, ResourceLoader_arena0));

  void *pFragmentFile;
  size_t fragmentFileSize;
  ASSERT_CODE_ALWAYS(ResourceLoader_load_file(
      fragmentPath, &pFragmentFile, &fragmentFileSize, ResourceLoader_arena1));

  if (!pRootSignature) {
    // get root signature from shader
    ComPtr<ID3DBlob> pVSRootSignatureBlob;
    ASSERT_WIN_ALWAYS(D3DGetBlobPart(pVertexFile, vertexFileSize,
                                     D3D_BLOB_ROOT_SIGNATURE, 0,
                                     &pVSRootSignatureBlob));

    ComPtr<ID3DBlob> pPSRootSignatureBlob;
    ASSERT_WIN_ALWAYS(D3DGetBlobPart(pFragmentFile, fragmentFileSize,
                                     D3D_BLOB_ROOT_SIGNATURE, 0,
                                     &pPSRootSignatureBlob));

    bool rootSignaturesEqual = false;
    if (pVSRootSignatureBlob.Get() && pPSRootSignatureBlob.Get() &&
        pVSRootSignatureBlob->GetBufferSize() ==
            pPSRootSignatureBlob->GetBufferSize()) {
      rootSignaturesEqual = memcmp(pVSRootSignatureBlob->GetBufferPointer(),
                                   pPSRootSignatureBlob->GetBufferPointer(),
                                   pVSRootSignatureBlob->GetBufferSize()) == 0;
    }

    ASSERT_WIN_ALWAYS(pDevice->CreateRootSignature(
        0, pVSRootSignatureBlob->GetBufferPointer(),
        pVSRootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&pRootSignature)));
  } else {
    pRootSignature->AddRef();
  }

  D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{
      .pRootSignature = pRootSignature,
      .VS =
          {
              .pShaderBytecode = pVertexFile,
              .BytecodeLength = vertexFileSize,
          },
      .PS =
          {
              .pShaderBytecode = pFragmentFile,
              .BytecodeLength = fragmentFileSize,
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

  ID3D12PipelineState *pPipelineState;

  ASSERT_WIN_ALWAYS(pDevice->CreateGraphicsPipelineState(
      &graphicsPipelineStateDesc, IID_PPV_ARGS(&pPipelineState)));

  ResourceLoader_arena0_reset();
  ResourceLoader_arena1_reset();

  Asset_ShaderData shaderData{
      .pPipelineState = pPipelineState,
      .pRootSignature = pRootSignature,
  };

  ShaderContainer *container;
  if (m_pFirstEmptyShader->BlockSize >= sizeof(ShaderContainer) * 2) {
    // split the block
    container = m_pFirstEmptyShader;
    ++m_pFirstEmptyShader;

    m_pFirstEmptyShader->BlockSize =
        container->BlockSize - sizeof(ShaderContainer);
    m_pFirstEmptyShader->pNextEmpty = container->pNextEmpty;
  } else {
    // next block
    container = m_pFirstEmptyShader;
    m_pFirstEmptyShader = container->pNextEmpty;
  }

  container->Data = shaderData;
  ++container->Version;

  return HAsset_Shader{
      .m_Index = (uint32_t)(container - m_pShaders),
      .m_Version = container->Version,
  };
}

// GPUImage AssetSystem::load_png(const char *path, bool raw) {
//   ResourceLoader_arena0_reset();
//   ResourceLoader_arena1_reset();
//
//   void *file;
//   size_t fileSize;
//   ASSERT_WIN_CODE_ALWAYS(
//       ResourceLoader_load_file(path, &file, &fileSize,
//       ResourceLoader_arena0), "Failed to load file: %s", path);
//
//   PngInfo png;
//   ASSERT_CODE_ALWAYS(ResourceLoader_decompress_png(file, fileSize, &png,
//                                                    ResourceLoader_arena1));
//   ResourceLoader_arena0_reset();
//
//   ResourceLoader_arena_t nextArena = ResourceLoader_arena0;
//   if (png.InterlaceMethod != 0) {
//     ASSERT_CODE_ALWAYS(
//         ResourceLoader_deinterlace_png(&png, ResourceLoader_arena0));
//     nextArena = ResourceLoader_arena1;
//   }
//
//   ASSERT_CODE_ALWAYS(ResourceLoader_png_to_rgba8(&png, nextArena));
//
//   ASSERT(png.ColorType == 6);
//   ASSERT(png.BitDepth == 8);
//   ASSERT(png.InterlaceMethod == 0);
//
//   DXGI_FORMAT format;
//   if (raw)
//     format = DXGI_FORMAT_R8G8B8A8_UNORM;
//   else
//     format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
//
//   ID3D12Device9 *pDevice = g_RenderingSystem.get_device();
//
//   // Check allocation info
//   D3D12_RESOURCE_DESC desc{
//       .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
//       .Alignment = 0,
//       .Width = png.Width,
//       .Height = png.Height,
//       .DepthOrArraySize = 1,
//       .MipLevels = 1,
//       .Format = format,
//       .SampleDesc = {1, 0},
//       .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
//       .Flags = D3D12_RESOURCE_FLAG_NONE,
//   };
//
//   D3D12_RESOURCE_ALLOCATION_INFO info =
//       pDevice->GetResourceAllocationInfo(0, 1, &desc);
//
//   desc.Alignment = info.Alignment;
//
//   // Check if has room
//   size_t alignedOffset =
//       ((m_LevelHeapOffset + info.Alignment - 1) / info.Alignment) *
//       info.Alignment;
//   m_LevelHeapOffset = alignedOffset + info.SizeInBytes;
//
//   if (m_LevelHeapOffset > m_LevelHeapCapacity) {
//     CRASH("Out of room in level data heap!");
//   }
//
//   if (m_LevelDescriptorCount + 1 > m_LevelResourceCapacity) {
//     CRASH("Out of level descriptors!");
//   }
//
//   // Get descriptor handles
//   D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle =
//       m_LevelDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
//   cpuHandle.ptr +=
//       m_LevelDescriptorCount * pDevice->GetDescriptorHandleIncrementSize(
//                                    D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
//
//   D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle =
//       m_LevelDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
//   gpuHandle.ptr +=
//       m_LevelDescriptorCount * pDevice->GetDescriptorHandleIncrementSize(
//                                    D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
//
//   // Create resource
//   ASSERT_WIN_ALWAYS(pDevice->CreatePlacedResource(
//       m_LevelHeap.Get(), alignedOffset, &desc,
//       D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
//       IID_PPV_ARGS(&m_LevelResources[m_LevelResourceCount])));
//
//   // Create view
//   D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc{
//       .Format = format,
//       .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
//       .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
//       .Texture2D =
//           {
//               .MostDetailedMip = 0,
//               .MipLevels = 1,
//               .PlaneSlice = 0,
//               .ResourceMinLODClamp = 0,
//           },
//   };
//   pDevice->CreateShaderResourceView(m_LevelResources[m_LevelResourceCount],
//                                     &viewDesc, cpuHandle);
//
//   UINT pitch = ((png.Width * 4 + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1) /
//                 D3D12_TEXTURE_DATA_PITCH_ALIGNMENT) *
//                D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;
//
//   if ((size_t)pitch * png.Height > m_StagingBufferCapacity) {
//     CRASH("Staging buffer too small!");
//   }
//
//   // Copy into staging buffer
//   for (size_t y = 0; y < png.Height; ++y) {
//     for (size_t x = 0; x < png.Width; ++x) {
//       size_t i = y * pitch + x * 4;
//       size_t j = (png.Height - y - 1) * png.Width * 4 + x * 4;
//       m_StagingBufferMap[i + 0] = png.Data[j + 0];
//       m_StagingBufferMap[i + 1] = png.Data[j + 1];
//       m_StagingBufferMap[i + 2] = png.Data[j + 2];
//       m_StagingBufferMap[i + 3] = png.Data[j + 3];
//     }
//   }
//
//   // Copy from staging buffer
//   ID3D12GraphicsCommandList10 *pList =
//   g_RenderingSystem.record_staging_list();
//
//   D3D12_TEXTURE_COPY_LOCATION dst{
//       .pResource = m_LevelResources[m_LevelResourceCount],
//       .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
//       .SubresourceIndex = 0,
//   };
//   D3D12_TEXTURE_COPY_LOCATION src{
//       .pResource = m_StagingBuffer.Get(),
//       .Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
//       .PlacedFootprint =
//           {
//               .Offset = 0,
//               .Footprint =
//                   {
//                       .Format = format,
//                       .Width = png.Width,
//                       .Height = png.Height,
//                       .Depth = 1,
//                       .RowPitch = pitch,
//                   },
//           },
//   };
//   pList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
//
//   D3D12_RESOURCE_BARRIER barrier{
//       .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
//       .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
//       .Transition = {
//           .pResource = m_LevelResources[m_LevelResourceCount],
//           .Subresource = 0,
//           .StateBefore = D3D12_RESOURCE_STATE_COPY_DEST,
//           .StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
//                         D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
//       }};
//   pList->ResourceBarrier(1, &barrier);
//
//   g_RenderingSystem.execute_staging_list();
//
//   ++m_LevelDescriptorCount;
//   ++m_LevelResourceCount;
//
//   ResourceLoader_arena0_reset();
//   ResourceLoader_arena1_reset();
//
//   return GPUImage(gpuHandle, m_LevelDescriptorHeap.Get(), png.Width,
//                   png.Height);
// }