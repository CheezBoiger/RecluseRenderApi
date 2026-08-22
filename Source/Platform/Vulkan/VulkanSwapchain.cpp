//
#include "VulkanSwapchain.hpp"
#include "VulkanDevice.hpp"
#include "VulkanAdapter.hpp"
#include "VulkanInstance.hpp"

namespace Recluse {
namespace RenderApi {
namespace Vulkan {


VulkanSwapchain::VulkanSwapchain(VulkanDevice* device, VkSurfaceKHR surface, const Swapchain::Description& description)
    : m_swapchain(VK_NULL_HANDLE)
    , m_description(description)
    , m_device(VK_NULL_HANDLE)
    , m_surface(surface)
{
    R_ASSERT(device);
    m_device = device->get();
    initialize(device);   
}

void VulkanSwapchain::initialize(VulkanDevice* device)
{
    if (!device) return;
    if (!m_surface) return;

    VkPhysicalDevice physicalDevice                             = device->getAdapter()->get();
    VkSurfaceCapabilitiesKHR surfaceCapabilities                = { };
    VkFormat requestedSurfaceFormat                             = getVulkanFormat(m_description.format);
    VkColorSpaceKHR requestedColorSpace                         = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR; //m_description.colorSpace;
    VkColorSpaceKHR colorSpace                                  = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    VkFormat actualFormat                                       = VK_FORMAT_UNDEFINED;
    VkImageUsageFlags imageUsageFlags                           = 0;
    VkPresentModeKHR presentMode                                = VK_PRESENT_MODE_FIFO_KHR;

    switch (m_description.presentMode)
    {
        case PresentMode_Immediate:
            presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            break;
        case PresentMode_TripleBuffering:
            presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        default:
        case PresentMode_DoubleBuffering:
            presentMode = VK_PRESENT_MODE_FIFO_KHR;
            break;
    }

    VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_surface, &surfaceCapabilities);
    R_ASSERT(result == VK_SUCCESS);
    
    {
        std::vector<VkSurfaceFormatKHR> compatibleSurfaceFormats    = { };
        uint surfaceFormatCount = 0;
        result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_surface, &surfaceFormatCount, nullptr);
        R_ASSERT(result == VK_SUCCESS);
        compatibleSurfaceFormats.resize(surfaceFormatCount);
        result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_surface, &surfaceFormatCount, compatibleSurfaceFormats.data());
        R_ASSERT(result == VK_SUCCESS);

        for (uint i = 0; i < compatibleSurfaceFormats.size(); ++i)
        {
            const VkSurfaceFormatKHR& surfaceFormat = compatibleSurfaceFormats[i];
            if (requestedSurfaceFormat == surfaceFormat.format &&
                requestedColorSpace == surfaceFormat.colorSpace)
            {
                actualFormat = surfaceFormat.format;
                colorSpace = surfaceFormat.colorSpace;
                break;            
            }
        }

        if (actualFormat == VK_FORMAT_UNDEFINED)
        {
            // Default to the first surface format.
            actualFormat = compatibleSurfaceFormats[0].format;
            colorSpace = compatibleSurfaceFormats[0].colorSpace;
        }
    }

    {
        std::vector<VkPresentModeKHR> compatiblePresentModes        = { };
        uint presentModeCount = 0;
        result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_surface, &presentModeCount, nullptr);
        R_ASSERT(result == VK_SUCCESS);
        compatiblePresentModes.resize(presentModeCount);
        result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_surface, &presentModeCount, compatiblePresentModes.data());
        R_ASSERT(result == VK_SUCCESS);
    
        Bool isSupported = false;
        for (uint i = 0; i < presentModeCount; ++ i)
        {
            VkPresentModeKHR mode = compatiblePresentModes[i];
            if (presentMode == mode)
            {
                isSupported = true;
                break;
            }
        }

        if (!isSupported)
        {
            // the present mode requested is not compatible with the surface, use default instead.
            presentMode = compatiblePresentModes[0];
        }
    }

    ResourceUsageFlags actualUsageFlags = 0;

    // Should be standard.
    if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
    {
        imageUsageFlags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        actualUsageFlags |= ResourceUsage_RenderTarget;
    }

    if (m_description.usage & ResourceUsage_ShaderResource)
    {
        if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_SAMPLED_BIT)
        {
            imageUsageFlags |= VK_IMAGE_USAGE_SAMPLED_BIT;
            actualUsageFlags |= ResourceUsage_ShaderResource;
        }
    }

    VkSwapchainCreateInfoKHR swapchainCi    = { };
    swapchainCi.sType                       = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCi.surface                     = m_surface;
    swapchainCi.presentMode                 = presentMode;
    swapchainCi.imageFormat                 = actualFormat;
    swapchainCi.imageColorSpace             = colorSpace;
    swapchainCi.imageUsage                  = actualUsageFlags;
    swapchainCi.preTransform                = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchainCi.compositeAlpha              = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCi.imageArrayLayers            = 1;
    swapchainCi.oldSwapchain                = m_swapchain ? m_swapchain : VK_NULL_HANDLE;
    swapchainCi.clipped                     = VK_TRUE;
    swapchainCi.imageExtent.width           = Math::clamp(m_description.renderWidth, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
    swapchainCi.imageExtent.height          = Math::clamp(m_description.renderHeight, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
    swapchainCi.minImageCount               = Math::clamp(m_description.numFrames, surfaceCapabilities.minImageCount, surfaceCapabilities.maxImageCount);
    swapchainCi.pQueueFamilyIndices         = nullptr;
    swapchainCi.queueFamilyIndexCount       = 0;
 
    result = vkCreateSwapchainKHR(m_device, &swapchainCi, nullptr, &m_swapchain);

    if (result == VK_SUCCESS)
    {
        m_description.format        = getResourceFormat(actualFormat);
        m_description.usage         = actualUsageFlags;
        m_description.renderWidth   = swapchainCi.imageExtent.width;
        m_description.renderHeight  = swapchainCi.imageExtent.height;
        m_description.numFrames     = swapchainCi.minImageCount;
    }
}

void VulkanSwapchain::release()
{
    if (m_swapchain)
    {
        vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
    }

    m_swapchain = VK_NULL_HANDLE;
}

ResultCode VulkanSwapchain::rebuild(const Swapchain::Description& description)
{
    return RecluseResult_NoImpl;
}

Resource* VulkanSwapchain::currentBackbuffer()
{
    return nullptr;
}

Swapchain::Description VulkanSwapchain::getDescription()
{
    return m_description;
}
} // Vulkan
} // RenderApi 
} // Recluse