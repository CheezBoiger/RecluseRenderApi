#ifndef RECLUSE_VULKAN_DEVICE_HPP
#define RECLUSE_VULKAN_DEVICE_HPP

#pragma once

#include "VulkanCommon.hpp"

#include <Recluse/RenderApi/Adapter.hpp>
#include <Recluse/RenderApi/Device.hpp>
#include <RecluseRenderApi_exports.hpp>

namespace Recluse {
namespace RenderApi {
namespace Vulkan {

class VulkanAdapter;

class VulkanDevice : public Device
{
public:
    VulkanDevice(VulkanAdapter* adapter, VkDevice device = VK_NULL_HANDLE);

    ~VulkanDevice();

    virtual ResultCode  createResource(const Resource::Description& description, Resource** pResourceOut, 
        void* pInitialData, uint initialSizeBytes) override;
    virtual ResultCode  createQueue(const Queue::Description& description, Queue** ppQueueOut) override;
    virtual ResultCode  createPipeline(const PipelineDescription& description, Pipeline** pPipelineOut) override;

    // Create a swapchain for presentation. Requires a created queue for this.
    virtual ResultCode  createSwapchain(const SwapchainCreateDescription& description, Swapchain** ppSwapchainOut) override;

    virtual ResultCode  freeSwapchain(Swapchain* swapchain) override;
    virtual ResultCode  freePipeline(Pipeline* pipeline) override;
    virtual ResultCode  freeQueue(Queue* queue) override;

    VkDevice            operator()() const { return m_device; }
    VkDevice            get() const { return m_device; }

    VulkanAdapter*      getAdapter() const { return m_adapter; }

private:
    VkDevice            m_device;
    VulkanAdapter*      m_adapter;
};
} // Vulkan
} // RenderApi
} // Recluse
#endif // RECLUSE_VULKAN_DEVICE_HPP