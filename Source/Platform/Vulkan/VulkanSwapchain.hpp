#ifndef RECLUSE_VULKAN_SWAPCHAIN_HPP
#define RECLUSE_VULKAN_SWAPCHAIN_HPP

#pragma once

#include <Recluse/RenderApi/Swapchain.hpp>

#include "VulkanCommon.hpp"

namespace Recluse {
namespace RenderApi {
namespace Vulkan {

class VulkanDevice;

class VulkanSwapchain : public Swapchain
{
    struct Buffer
    {
        VkSemaphore waitSemaphore;
        VkSemaphore signalSemaphore;
        VkFence     fence;
        VkImage     image;              // The actual image itself.
    };

public:

    VulkanSwapchain(VulkanDevice* device = nullptr, VkSurfaceKHR surface = VK_NULL_HANDLE, const Swapchain::Description& swapchainDescription = { });

    ResultCode                  rebuild(const Swapchain::Description& description) override;
    Resource*                   currentBackbuffer() override;
    Swapchain::Description      getDescription() override;

    void                        initialize(VulkanDevice* device);
    void                        release();
    VkSurfaceKHR                getSurface() const { return m_surface; }

    Bool                        isValid() const override { return (m_swapchain != VK_NULL_HANDLE); }
    
private:
    VkSurfaceKHR                m_surface;
    VkSwapchainKHR              m_swapchain;
    Swapchain::Description      m_description;
    VkDevice                    m_device;
    std::vector<Buffer>         m_backBuffers;
};
} // Vulkan
} // RenderApi
} // Recluse
#endif // RECLUSE_VULKAN_SWAPCHAIN_HPP