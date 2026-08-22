#ifndef RECLUSE_VULKAN_DEVICE_HPP
#define RECLUSE_VULKAN_DEVICE_HPP

#pragma once

#include "VulkanCommon.hpp"
#include "VulkanSwapchain.hpp"

#include <Recluse/RenderApi/Adapter.hpp>
#include <Recluse/RenderApi/Device.hpp>

#include <RecluseRenderApi_exports.hpp>
#include <vector>
#include <map>

namespace Recluse {
namespace RenderApi {
namespace Vulkan {

class VulkanAdapter;

class RecluseRenderApi_PUBLIC_API VulkanDevice : public Device
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

    virtual Resource*       createResource(const Resource::Description& description, 
        void* pInitialData, uint initialSizeBytes) override;
    virtual Pipeline*       createPipeline(const PipelineDescription& description) override;
    virtual Fence           createFence() override;

    // Create a swapchain for presentation. Requires a created queue for this.
    virtual Swapchain*      createSwapchain(const Swapchain::Description& description) override;

    virtual ResultCode      freeSwapchain(Swapchain* swapchain) override;
    virtual ResultCode      freePipeline(Pipeline* pipeline) override;
    virtual ResultCode      freeFence(Fence fence) override;

    virtual FrameProcess*   createFrameProcess(const FrameProcess::Description& description) override;
    virtual ResultCode      freeFrameProcess(FrameProcess* frameProcess) override;
    virtual ResultCode      processFrame(FrameHandle frame) override;

    VkDevice                operator()() const { return m_device; }
    VkDevice                get() const { return m_device; }

    VulkanAdapter*          getAdapter() const { return m_adapter; }
    
    void                    release();

    VkQueue                 queryQueue(CommandQueueType queueType);

    VkFence                 internalCreateFence(const VkFenceCreateInfo& ci); 
    void                    internalFreeFence(VkFence fence);

private:
    VkDevice                m_device;
    VulkanAdapter*          m_adapter;

    // Individual FamilyInfo Maps.
    QueueIndices            m_queueIndices;

    VkQueue                 m_graphicsQueue;
    VkQueue                 m_computeQueue;
    VkQueue                 m_copyQueue;

    std::map<VkSurfaceKHR, VulkanSwapchain> m_swapchainMap;
};
} // Vulkan
} // RenderApi
} // Recluse
#endif // RECLUSE_VULKAN_DEVICE_HPP