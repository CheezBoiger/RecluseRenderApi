// Recluse Render Api.
#ifndef RECLUSE_RENDER_API_COMMON_HPP
#define RECLUSE_RENDER_API_COMMON_HPP

#pragma once

#include <Recluse/Types.hpp>

namespace Recluse {
namespace RenderApi {

typedef U32 PipelineId;
typedef U32 ResourceId;
typedef U32 SamplerId;

typedef U64 ResourceViewId;

enum Api
{
    Unknown,
    Direct3D11,
    Direct3D12,
    Vulkan,
    OpenGL,
    SoftwareRaster,
    SoftwareRaytrace,
};


enum FeatureFlag
{
    FeatureFlag_None = 0,
    FeatureFlag_Raytracing = 1 << 0,
    FeatureFlag_MeshShader = 1 << 1,
    FeatureFlag_DebugMarkers = 1 << 2,
};

typedef u32 FeatureFlags;

enum ResourceFormat
{
    ResourceFormat_Unknown,
    ResourceFormat_R8G8B8A8_UNORM,
    ResourceFormat_R8G8B8A8_SNORM,
    ResourceFormat_R16G16B16A16_FLOAT,
    ResourceFormat_R32G32B32A32_FLOAT,
    ResourceFormat_D24_UNORM_S8_UINT,
    ResourceFormat_D32_FLOAT_S8X24_UINT,
};


enum ResourceType
{
    ResourceType_Unknown,
    ResourceType_Buffer,
    ResourceType_Texture1D,
    ResourceType_Texture1DArray,
    ResourceType_Texture2D,
    ResourceType_Texture2DArray,
    ResourceType_Texture3D,
    ResourceType_TextureCube,
    ResourceType_TextureCubeArray,
};


enum ResourceMemoryUsage
{
    ResourceMemoryUsage_Unknown,
    // Default is the default memory usage for the resource. This is typically GPU local memory.
    ResourceMemoryUsage_Default,
    // Immutable is memory that is allocated on the GPU and initialized once, but cannot be written to by the CPU. This is typically used for resources that are static and do not change often.
    ResourceMemoryUsage_Immutable,
    // Dynamic is memory that is allocated on the CPU and can be written to by the CPU. This is typically used for resources that change often.
    ResourceMemoryUsage_Dynamic,
    // Staging is memory that is allocated on the CPU and can be written to by the CPU. This is typically used for resources that are used for transferring data between the CPU and GPU.
    ResourceMemoryUsage_Staging,
};

enum ResourceUsage
{
    ResourceUsage_VertexBuffer          = (1 << 0),
    ResourceUsage_IndexBuffer           = (1 << 1),
    ResourceUsage_RenderTarget          = (1 << 2),
    ResourceUsage_ShaderResource        = (1 << 3),
    ResourceUsage_ConstantBuffer        = (1 << 4),
    ResourceUsage_TransferDestination   = (1 << 5),
    ResourceUsage_CopyDestination       = ResourceUsage_TransferDestination,
    ResourceUsage_TransferSource        = (1 << 6),
    ResourceUsage_CopySource            = ResourceUsage_TransferSource,
    ResourceUsage_IndirectBuffer        = (1 << 7),
    ResourceUsage_DepthStencil          = (1 << 8),
    ResourceUsage_UnorderedAccess       = (1 << 9),
    ResourceUsage_AccelerationStructure = (1 << 10)
};


struct InstanceDescription
{
};


struct ResourceDescription
{
    ResourceType type;
    ResourceFormat format;
    ResourceUsage usage;
    ResourceMemoryUsage memoryUsage;
    uint width;
    uint height;
    uint depthOrArraySize;
};


struct DescriptorSet
{
public:
    
};

enum ResourceState
{
    ResourceState_Unknown,
    ResourceState_VertexBuffer,
    ResourceState_IndexBuffer,
    ResourceState_RenderTarget,
    ResourceState_ShaderResource,
    ResourceState_ConstantBuffer,
    ResourceState_TransferDestination,
    ResourceState_CopyDestination = ResourceState_TransferDestination,
    ResourceState_TransferSource,
    ResourceState_CopySource = ResourceState_TransferSource,
    ResourceState_IndirectBuffer,
    ResourceState_DepthStencil,
    ResourceState_UnorderedAccess,
};

enum ResourceViewType
{
    ResourceViewType_Unknown,
    ResourceViewType_RenderTarget,
    ResourceViewType_DepthStencil,
    ResourceViewType_ShaderResource,
    ResourceViewType_UnorderedAccess,
    ResourceViewType_ConstantBuffer,
};

enum ResourceViewDimension
{
    ResourceViewDimension_Unknown,
    ResourceViewDimension_1D,
    ResourceViewDimension_2D,
    ResourceViewDimension_3D,
    ResourceViewDimension_Cube,
};

struct ResourceViewDescription
{
    ResourceViewType        type;
    ResourceViewDimension   dimension;
    ResourceFormat          format;
    uint                    width;
    uint                    height;
    uint                    depthOrArraySize;
};

enum CommandTypeFlag
{
    CommandTypeFlag_Unknown,
    CommandTypeFlag_Graphics = 1 << 0,
    CommandTypeFlag_Compute = 1 << 1,
    CommandTypeFlag_Copy = 1 << 2,
    CommandTypeFlag_Direct = (CommandTypeFlag_Compute | CommandTypeFlag_Copy | CommandTypeFlag_Graphics),
};

typedef u32 CommandTypeFlags;

struct QueueCreateDescription
{
    CommandTypeFlags typeFlags = CommandTypeFlag_Direct;
};
} // RenderApi
} // Recluse
#endif // RECLUSE_RENDER_API_COMMON_HPP