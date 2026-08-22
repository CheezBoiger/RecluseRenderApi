// Recluse Render Api.
#ifndef RECLUSE_RENDER_API_COMMON_HPP
#define RECLUSE_RENDER_API_COMMON_HPP

#pragma once

#include <Recluse/Types.hpp>

// Vendor IDs
#define NVIDIA_VENDOR_ID        0x10DE
#define INTEL_VENDOR_ID         0x8086
#define AMD_VENDOR_ID           0x1022
#define MSFT_VENDOR_ID          0x1414
#define QUALCOMM_VENDOR_ID      0x5143

namespace Recluse {
namespace RenderApi {

typedef U32 PipelineId;
typedef U32 ResourceId;
typedef U32 SamplerId;

typedef U64 ResourceViewId;
typedef u64 Fence;

enum class Api : u32
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
    FeatureFlag_EnableDebugMarkers = 1 << 2,
    FeatureFlag_EnableValidation = 1 << 3,
    FeatureFlag_EnableApiDump = 1 << 4,
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
    // Default is the default memory usage for the resource. This is typically GPU local memory, allowed to be written to by the GPU, but not CPU.
    ResourceMemoryUsage_Default,
    // Immutable is memory that is allocated on the GPU and initialized once, but cannot be written to by the CPU or GPU. 
    // This is typically used for resources that are static and do not change.
    ResourceMemoryUsage_Immutable,
    // Dynamic is memory that is allocated on the CPU and can be written to by the CPU. This is typically used for resources that change often.
    // Readable by the GPU, not writable. Should not be read by the CPU.
    ResourceMemoryUsage_Dynamic,
    // Staging is memory that is allocated on the CPU and can be written to by the CPU. This is typically used for resources that are transferring data from CPU to GPU.
    ResourceMemoryUsage_Staging,
    // Readback is memory that is readable by the CPU. Typically used for resources that are transferring data from GPU to CPU.
    ResourceMemoryUsage_Readback,
};

enum ResourceUsage
{
    ResourceUsage_VertexBuffer          = (1 << 0),
    ResourceUsage_IndexBuffer           = (1 << 1),
    ResourceUsage_RenderTarget          = (1 << 2),
    ResourceUsage_ShaderResource        = (1 << 3),
    ResourceUsage_ConstantBuffer        = (1 << 4),
    ResourceUsage_TransferDestination   = (1 << 5),
    ResourceUsage_TransferSource        = (1 << 6),
    ResourceUsage_IndirectBuffer        = (1 << 7),
    ResourceUsage_DepthStencil          = (1 << 8),
    ResourceUsage_UnorderedAccess       = (1 << 9),
    ResourceUsage_AccelerationStructure = (1 << 10)
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

enum CommandQueueType
{
    // Graphics queue. This is default.
    CommandQueueType_Graphics,
    // Asyncronous compute queue type.
    CommandQueueType_Compute,
    // Asyncronous copy queue type.
    CommandQueueType_Copy,
};

enum IndexType 
{
    IndexType_Unsigned16,
    IndexType_Unsigned32
};


struct Viewport 
{
    F32 x;
    F32 y;
    F32 width;
    F32 height;
    F32 minDepth;
    F32 maxDepth;
};

// Rectangle structure.
// x is the offset of the rectangle in x position
// y is the offset of the rectangle in y position
// width is the overall width extent of the rectangle.
// height is the overall height extent of the rectangle.
struct Rect 
{
    F32 x, 
        y, 
        width, 
        height;
};


enum ClearFlag
{
    ClearFlag_None = 0,
    ClearFlag_Color = 0x001,
    ClearFlag_Depth = 0x002,
    ClearFlag_Stencil = 0x004
};


typedef U32 ClearFlags;

struct IApiObject
{
public:
    // Checks of the object is valid.
    virtual Bool isValid() const = 0;
};
} // RenderApi
} // Recluse
#endif // RECLUSE_RENDER_API_COMMON_HPP