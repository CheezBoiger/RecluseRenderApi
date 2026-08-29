#ifndef RECLUSE_VULKAN_SWAPCHAIN_HPP
#define RECLUSE_VULKAN_SWAPCHAIN_HPP

#pragma once

#include <Recluse/RenderApi/Swapchain.hpp>

#include "VulkanResource.hpp"
#include "VulkanCommon.hpp"

namespace Recluse {
namespace RenderApi {
namespace Vulkan {

class VulkanDevice;

class VulkanSwapchain : public Swapchain
{
public:
    VulkanSwapchain(VulkanDevice* device = nullptr, VkSurfaceKHR surface = VK_NULL_HANDLE, const Swapchain::Description& swapchainDescription = { });
    virtual ~VulkanSwapchain();

    ResultCode                  rebuild(const Swapchain::Description& description) override;
    Resource*                   currentBackbuffer() override;
    Swapchain::Description      getDescription() override;

    // Initialize the swapchain.
    void                        initialize();
    VulkanResource&             getNativeResource(ResourceId id);

    // Release the swapchain, this is essential for destroying the swaphain resources.
    void                        release();

    VkSurfaceKHR                getSurface() const { return m_surface; }

    Bool                        isValid() const override { return (m_swapchain != VK_NULL_HANDLE); }
    uint                        currentImageIndex() const { return m_currentImageIndex; }
    uint                        aquireNextFrameIndex(VkSemaphore waitSemaphore);
    VkSemaphore                 currentSignalSemaphore() const { return m_signalSemaphores[currentImageIndex()]; }

    VkSwapchainKHR             get() const { return m_swapchain; }

private:
    void                        initializeSwapchainResources();
    void                        destroySwapchainResources();

    VkSurfaceKHR                 m_surface;
    VkSwapchainKHR               m_swapchain;
    Swapchain::Description       m_description;
    VkDevice                     m_device;
    VkPhysicalDevice             m_physicalDevice;
    std::vector<VkSemaphore>     m_signalSemaphores;
    std::vector<VulkanResource>  m_imageResources;
    uint                         m_currentImageIndex;
};
} // Vulkan
} // RenderApi
} // Recluse
#endif // RECLUSE_VULKAN_SWAPCHAIN_HPP