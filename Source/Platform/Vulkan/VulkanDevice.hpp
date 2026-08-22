#ifndef RECLUSE_VULKAN_DEVICE_HPP
#define RECLUSE_VULKAN_DEVICE_HPP

#pragma once

#include "VulkanCommon.hpp"

#include <Recluse/RenderApi/Adapter.hpp>
#include <Recluse/RenderApi/Device.hpp>

#include <RecluseRenderApi_exports.hpp>
#include <vector>
#include <map>

namespace Recluse {
namespace RenderApi {
namespace Vulkan {

class VulkanAdapter;

class VulkanDevice : public Device
{
public:
    struct QueueProperties
    {
        const static uint kBadIndex = ~0;
        uint familyIndex = kBadIndex;
        uint queueIndex = kBadIndex;
    };

    struct QueueIndices
    {
        QueueProperties graphics;
        QueueProperties compute;
        QueueProperties copy;
    };

    VulkanDevice(VulkanAdapter* adapter = nullptr, VkDevice device = VK_NULL_HANDLE, 
        const QueueIndices& indices = { });

    ~VulkanDevice();

    virtual ResultCode  createResource(const Resource::Description& description, Resource** pResourceOut, 
        void* pInitialData, uint initialSizeBytes) override;
    virtual ResultCode  createPipeline(const PipelineDescription& description, Pipeline** pPipelineOut) override;

    // Create a swapchain for presentation. Requires a created queue for this.
    virtual ResultCode  createSwapchain(const SwapchainCreateDescription& description, Swapchain** ppSwapchainOut) override;

    virtual ResultCode  freeSwapchain(Swapchain* swapchain) override;
    virtual ResultCode  freePipeline(Pipeline* pipeline) override;

    VkDevice            operator()() const { return m_device; }
    VkDevice            get() const { return m_device; }

    VulkanAdapter*      getAdapter() const { return m_adapter; }
    
    void                release();

    VkQueue             queryQueue(CommandQueueType queueType);

private:
    VkDevice            m_device;
    VulkanAdapter*      m_adapter;

    // Individual FamilyInfo Maps.
    QueueIndices        m_queueIndices;

    VkQueue             m_graphicsQueue;
    VkQueue             m_computeQueue;
    VkQueue             m_copyQueue;
};
} // Vulkan
} // RenderApi
} // Recluse
#endif // RECLUSE_VULKAN_DEVICE_HPP