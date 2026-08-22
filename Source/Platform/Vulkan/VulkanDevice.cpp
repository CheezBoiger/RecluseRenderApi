#include "VulkanDevice.hpp"
#include "VulkanAdapter.hpp"
#include "VulkanSwapchain.hpp"
#include "VulkanContext.hpp"

#include <Recluse/Messaging.hpp>

namespace Recluse {
namespace RenderApi {
namespace Vulkan {

VulkanDevice::VulkanDevice(VulkanAdapter* adapter, VkDevice device, 
    const QueueIndices& queueIndices)
    : m_device(device)
    , m_adapter(adapter)
    , m_queueIndices(queueIndices)
    , m_graphicsQueue(VK_NULL_HANDLE)
    , m_computeQueue(VK_NULL_HANDLE)
    , m_copyQueue(VK_NULL_HANDLE)
{
    R_ASSERT(m_device != VK_NULL_HANDLE);

    if (m_queueIndices.graphics.familyIndex != VulkanDevice::QueueProperties::kBadIndex)
    {
        vkGetDeviceQueue(m_device, m_queueIndices.graphics.familyIndex, m_queueIndices.graphics.queueIndex, &m_graphicsQueue);
    }

    if (m_queueIndices.compute.familyIndex != VulkanDevice::QueueProperties::kBadIndex)
    {
        vkGetDeviceQueue(m_device, m_queueIndices.compute.familyIndex, m_queueIndices.compute.queueIndex, &m_computeQueue);
    }

    if (m_queueIndices.copy.familyIndex != VulkanDevice::QueueProperties::kBadIndex)
    {
        vkGetDeviceQueue(m_device, m_queueIndices.copy.familyIndex, m_queueIndices.copy.queueIndex, &m_copyQueue);
    }
}

VulkanDevice::~VulkanDevice()
{
    m_device = VK_NULL_HANDLE;
}

void VulkanDevice::release()
{
    if (m_device)
    {
        vkDestroyDevice(m_device, nullptr);
    }
    m_device = VK_NULL_HANDLE;
}

Resource* VulkanDevice::createResource(const Resource::Description& description,
    void* pInitialData, uint initialSizeBytes)
{
    return nullptr;
}

Pipeline* VulkanDevice::createPipeline(const PipelineDescription& description)
{
    return nullptr;
}

Swapchain* VulkanDevice::createSwapchain(const Swapchain::Description& description)
{
    VkSurfaceKHR surface = getAdapter()->getContext()->makeSurface(description.windowHandle);
    auto it = m_swapchainMap.find(surface);
    if (it == m_swapchainMap.end())
    {
        m_swapchainMap.insert(std::make_pair(surface, VulkanSwapchain(this, surface, description)));
        return &m_swapchainMap[surface];
    }
    return nullptr;
}

ResultCode VulkanDevice::freeSwapchain(Swapchain* swapchain)
{
    if (!swapchain) return RecluseResult_NullPtrExcept;
    VulkanSwapchain* native = dynamic_cast<VulkanSwapchain*>(swapchain);
    if (native)
    {
        VkSurfaceKHR surface = native->getSurface();
        auto it = m_swapchainMap.find(surface);
        if (it != m_swapchainMap.end())
        {
            it->second.release();
            m_swapchainMap.erase(it);
            return RecluseResult_Ok;
        }
    }
    return RecluseResult_NotFound;
}

ResultCode VulkanDevice::freePipeline(Pipeline* pipeline)
{
    return RecluseResult_NoImpl;
}

Fence VulkanDevice::createFence()
{
    R_ASSERT(m_device);
    VkFenceCreateInfo info = { };
    VkFence fence = internalCreateFence(info);
    return reinterpret_cast<Fence>(fence);
}

ResultCode VulkanDevice::freeFence(Fence fence)
{
    if (fence == 0) return RecluseResult_NullPtrExcept;
    VkFence vkfence = reinterpret_cast<VkFence>(fence);
    internalFreeFence(vkfence);
    return RecluseResult_Ok;
}

VkFence VulkanDevice::internalCreateFence(const VkFenceCreateInfo& ci)
{
    VkFence fence = VK_NULL_HANDLE;
    VkResult result = vkCreateFence(m_device, &ci, nullptr, &fence);
    R_ASSERT(result == VK_SUCCESS);
    return fence;
}

void VulkanDevice::internalFreeFence(VkFence fence)
{
    R_ASSERT(m_device != VK_NULL_HANDLE);
    R_ASSERT(fence != VK_NULL_HANDLE);

    vkDestroyFence(m_device, fence, nullptr);
}

FrameProcess* VulkanDevice::createFrameProcess(const FrameProcess::Description& description)
{
    return nullptr;
}

ResultCode VulkanDevice::freeFrameProcess(FrameProcess* frameProcess)
{
    return RecluseResult_NoImpl;
}

ResultCode VulkanDevice::processFrame(FrameHandle frame)
{
    return RecluseResult_NoImpl;
}
} // Vulkan
} // RenderApi 
} // Recluse