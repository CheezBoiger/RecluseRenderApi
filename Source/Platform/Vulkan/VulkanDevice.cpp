#include "VulkanDevice.hpp"
#include "VulkanAdapter.hpp"
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

ResultCode VulkanDevice::createResource(const Resource::Description& description, Resource** resource,
    void* pInitialData, uint initialSizeBytes)
{
    return RecluseResult_NoImpl;
}

ResultCode VulkanDevice::createPipeline(const PipelineDescription& description, Pipeline** pPipelineOut)
{
    return RecluseResult_NoImpl;
}

ResultCode VulkanDevice::createSwapchain(const SwapchainCreateDescription& description, Swapchain** ppSwapchain)
{
    return RecluseResult_NoImpl;
}

ResultCode VulkanDevice::freeSwapchain(Swapchain* swapchain)
{
    return RecluseResult_NoImpl;
}

ResultCode VulkanDevice::freePipeline(Pipeline* pipeline)
{
    return RecluseResult_NoImpl;
}

} // Vulkan
} // RenderApi 
} // Recluse