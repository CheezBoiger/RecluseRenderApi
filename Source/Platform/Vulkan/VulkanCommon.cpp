//
#include "VulkanCommon.hpp"

namespace Recluse {
namespace RenderApi {
namespace Vulkan {

VkFormat getVulkanFormat(ResourceFormat format) 
{
    switch (format) 
    {
        case ResourceFormat_B8G8R8A8_Srgb:
            return VK_FORMAT_B8G8R8A8_SRGB;
        case ResourceFormat_R8G8B8A8_Unorm:
            return VK_FORMAT_R8G8B8A8_UNORM;
        case ResourceFormat_R16G16B16A16_Float:
            return VK_FORMAT_R16G16B16A16_SFLOAT;
        case ResourceFormat_R11G11B10_Float:
            return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
        case ResourceFormat_D32_Float:
            return VK_FORMAT_D32_SFLOAT;
        case ResourceFormat_R32_Float:
            return VK_FORMAT_R32_SFLOAT;
        case ResourceFormat_D24_Unorm_S8_Uint:
            return VK_FORMAT_D24_UNORM_S8_UINT;
        case ResourceFormat_R16G16_Float:
            return VK_FORMAT_R16G16_SFLOAT;
        case ResourceFormat_R32G32B32A32_Float:
            return VK_FORMAT_R32G32B32A32_SFLOAT;
        case ResourceFormat_R32G32B32A32_Uint:
            return VK_FORMAT_R32G32B32A32_UINT;
        case ResourceFormat_R32G32_Float:
            return VK_FORMAT_R32G32_SFLOAT;
        case ResourceFormat_R32G32_Uint:
            return VK_FORMAT_R32G32_UINT;
        case ResourceFormat_D32_Float_S8_Uint:
            return VK_FORMAT_D32_SFLOAT_S8_UINT;
        case ResourceFormat_R8_Uint:
            return VK_FORMAT_R8_UINT;
        case ResourceFormat_B8G8R8A8_Unorm:
            return VK_FORMAT_B8G8R8A8_UNORM;
        case ResourceFormat_R32G32B32_Float:
            return VK_FORMAT_R32G32B32_SFLOAT;
        case ResourceFormat_R32_Uint:
            return VK_FORMAT_R32_UINT;
        case ResourceFormat_R32_Int:
            return VK_FORMAT_R32_SINT;
        case ResourceFormat_BC1_Unorm:
            return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
        case ResourceFormat_BC2_Unorm:
            return VK_FORMAT_BC2_UNORM_BLOCK;
        case ResourceFormat_BC3_Unorm:
            return VK_FORMAT_BC3_UNORM_BLOCK;
        case ResourceFormat_BC4_Unorm:
            return VK_FORMAT_BC4_UNORM_BLOCK;
        case ResourceFormat_BC5_Unorm:
            return VK_FORMAT_BC5_UNORM_BLOCK;
        case ResourceFormat_BC7_Unorm:
            return VK_FORMAT_BC7_UNORM_BLOCK;
        case ResourceFormat_R24_Unorm_X8_Typeless:
            return VK_FORMAT_D24_UNORM_S8_UINT;
        case ResourceFormat_X24_Typeless_S8_Uint:
            return VK_FORMAT_D24_UNORM_S8_UINT;
        case ResourceFormat_D16_Unorm:
            return VK_FORMAT_D16_UNORM;
        default:
            return VK_FORMAT_UNDEFINED;
    }
}


ResourceFormat getResourceFormat(VkFormat format)
{
    switch (format)
    {
    case VK_FORMAT_R8G8B8A8_UNORM:
        return ResourceFormat_R8G8B8A8_Unorm;
    case VK_FORMAT_R16G16B16A16_SFLOAT:
        return ResourceFormat_R16G16B16A16_Float;
    case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
        return ResourceFormat_R11G11B10_Float;
    case VK_FORMAT_D32_SFLOAT:
        return ResourceFormat_D32_Float;
    case VK_FORMAT_R32_SFLOAT:
        return ResourceFormat_R32_Float;
    case VK_FORMAT_D24_UNORM_S8_UINT:
        return ResourceFormat_D24_Unorm_S8_Uint;
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        return ResourceFormat_D32_Float_S8_Uint;
    case VK_FORMAT_R16G16_SFLOAT:
        return ResourceFormat_R16G16_Float;
    case VK_FORMAT_B8G8R8A8_SRGB:
        return ResourceFormat_B8G8R8A8_Srgb;
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        return ResourceFormat_R32G32B32A32_Float;
    case VK_FORMAT_R32G32B32A32_UINT:
        return ResourceFormat_R32G32B32A32_Uint;
    case VK_FORMAT_R8_UINT:
        return ResourceFormat_R8_Uint;
    case VK_FORMAT_R32G32_SFLOAT:
        return ResourceFormat_R32G32_Float;
    case VK_FORMAT_R32G32_UINT:
        return ResourceFormat_R32G32_Uint;
    case VK_FORMAT_R16_UINT:
        return ResourceFormat_R16_Uint;
    case VK_FORMAT_R16_SFLOAT:
        return ResourceFormat_R16_Uint;
    case VK_FORMAT_B8G8R8A8_UNORM:
        return ResourceFormat_B8G8R8A8_Unorm;
    case VK_FORMAT_R32G32B32_SFLOAT:
        return ResourceFormat_R32G32B32_Float;
    case VK_FORMAT_R32_UINT:
        return ResourceFormat_R32_Uint;
    case VK_FORMAT_R32_SINT:
        return ResourceFormat_R32_Int;
    case VK_FORMAT_D16_UNORM:
        return ResourceFormat_D16_Unorm;
    case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
        return ResourceFormat_BC1_Unorm;
    case VK_FORMAT_BC2_UNORM_BLOCK:
        return ResourceFormat_BC2_Unorm;
    case VK_FORMAT_BC3_UNORM_BLOCK:
        return ResourceFormat_BC3_Unorm;
    case VK_FORMAT_BC4_UNORM_BLOCK:
        return ResourceFormat_BC4_Unorm;
    case VK_FORMAT_BC5_UNORM_BLOCK:
        return ResourceFormat_BC5_Unorm;
    case VK_FORMAT_BC7_UNORM_BLOCK:
        return ResourceFormat_BC7_Unorm;
    default:
        return ResourceFormat_Unknown;
    }
}

} // Vulkan
} // RenderApi 
} // Recluse