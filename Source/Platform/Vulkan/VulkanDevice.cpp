#include "VulkanDevice.hpp"
#include "VulkanAdapter.hpp"
#include <Recluse/Messaging.hpp>

namespace Recluse {
namespace RenderApi {
namespace Vulkan {

VulkanDevice::VulkanDevice(VulkanAdapter* adapter, VkDevice device)
    : m_device(device)
    , m_adapter(adapter)
{
    R_ASSERT(m_device != VK_NULL_HANDLE);
}

VulkanDevice::~VulkanDevice()
{
    m_device = VK_NULL_HANDLE;
}

ResultCode VulkanDevice::createResource(const Resource::Description& description, Resource** resource)
{
    return RecluseResult_NoImpl;
}

ResultCode VulkanDevice::createQueue(const Queue::Description& description, Queue** queue)
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

ResultCode VulkanDevice::freeQueue(Queue* queue)
{
    return RecluseResult_NoImpl;
}
} // Vulkan
} // RenderApi 
} // Recluse