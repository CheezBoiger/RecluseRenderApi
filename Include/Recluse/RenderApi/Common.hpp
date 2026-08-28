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

typedef U32     PipelineId;
typedef U32     SamplerId;

typedef U64     ResourceViewId;
typedef UPtr    Fence;
typedef void*   WindowHandle;

class ResourceId
{
public:
    static const uint kBadId = ~0;
    uint value = kBadId;
};

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

enum ResourceFormat
{
    ResourceFormat_Unknown,
    ResourceFormat_R8G8B8A8_Unorm,
    ResourceFormat_R16G16B16A16_Float,
    ResourceFormat_R11G11B10_Float,
    ResourceFormat_D32_Float,
    ResourceFormat_R32_Float,
    ResourceFormat_D16_Unorm,
    ResourceFormat_D24_Unorm_S8_Uint,
    ResourceFormat_D32_Float_S8_Uint,
    ResourceFormat_R24_Unorm_X8_Typeless,
    ResourceFormat_X24_Typeless_S8_Uint,
    ResourceFormat_R16G16_Float,
    ResourceFormat_B8G8R8A8_Srgb,
    ResourceFormat_R32G32B32A32_Float,
    ResourceFormat_R32G32B32A32_Uint,
    ResourceFormat_R8_Uint,
    ResourceFormat_R32G32_Float,
    ResourceFormat_R32G32_Uint,
    ResourceFormat_R16_Uint,
    ResourceFormat_R16_Float,
    ResourceFormat_B8G8R8A8_Unorm,
    ResourceFormat_R32G32B32_Float,
    ResourceFormat_R32_Uint,
    ResourceFormat_R32_Int,
    ResourceFormat_BC1_Unorm,
    ResourceFormat_BC2_Unorm,
    ResourceFormat_BC3_Unorm,
    ResourceFormat_BC4_Unorm,
    ResourceFormat_BC5_Unorm,
    ResourceFormat_BC7_Unorm,
};

enum ColorSpace
{
    ColorSpace_Unknown,
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
    ResourceUsage_IndirectBuffer        = (1 << 5),
    ResourceUsage_DepthStencil          = (1 << 6),
    ResourceUsage_UnorderedAccess       = (1 << 7),
    ResourceUsage_AccelerationStructure = (1 << 8)
};

typedef U32 ResourceUsageFlags;

struct DescriptorSet
{
public:
    
};

enum ResourceState
{
    ResourceState_Unknown,
    ResourceState_Common,
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
    ResourceState_DepthStencilReadOnly,
    ResourceState_DepthStencilWrite,
    ResourceState_UnorderedAccess,
    ResourceState_Present,
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