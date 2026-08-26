#include "VulkanDevice.hpp"
#include "VulkanAdapter.hpp"
#include "VulkanSwapchain.hpp"
#include "VulkanContext.hpp"
#include "VulkanFrameProcess.hpp"

#include <Recluse/Messaging.hpp>

#include <list>
#include <map>

namespace Recluse {
namespace RenderApi {
namespace Vulkan {

std::map<VulkanDevice*, std::list<VulkanFrameProcess*>> g_frameProcessMap;

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
    auto& it = g_frameProcessMap[this];
    
    it.push_back(new VulkanFrameProcess(get(), m_queueIndices, description));

    return it.back();
}

ResultCode VulkanDevice::freeFrameProcess(FrameProcess* frameProcess)
{
    VulkanFrameProcess* nativeProcess = dynamic_cast<VulkanFrameProcess*>(frameProcess);
    if (nativeProcess)
    {
        for (auto& it = g_frameProcessMap[this].begin(); it != g_frameProcessMap[this].end(); ++it)
        {
            if (nativeProcess == *it)
            {
                g_frameProcessMap[this].erase(it);
                nativeProcess->release();
                delete nativeProcess;
                return RecluseResult_Ok;
            }   
        }
    }
    return RecluseResult_NotFound;
}

ResultCode VulkanDevice::processFrame(FrameHandle frame)
{
    if (frame == 0) return RecluseResult_NullPtrExcept;
    
    VulkanFrameProcess::FrameStream* stream = reinterpret_cast<VulkanFrameProcess::FrameStream*>(frame);
    UPtr address = stream->baseAddress;
    const UPtr endAddress = address + stream->sizeBytes;

    while (address < endAddress)
    {
        uint sizeBytes = 0;
        VulkanFrameProcess::SubmitType submitType = *reinterpret_cast<VulkanFrameProcess::SubmitType*>(address);
        address += sizeof(VulkanFrameProcess::SubmitType);

        switch (submitType)
        {
            case VulkanFrameProcess::SubmitType_CommandBuffers:
            {
                const uint numCommandLists = *reinterpret_cast<uint*>(address);
                address += sizeof(uint);
                const uint numWaitSemaphores = *reinterpret_cast<uint*>(address);
                address += sizeof(uint);
                const uint numSignalSemaphores = *reinterpret_cast<uint*>(address);
                address += sizeof(uint);
                const CommandQueueType commandQueueType = *((CommandQueueType*)address);
                address += sizeof(CommandQueueType);
                UPtr data = *reinterpret_cast<UPtr*>(address);
                address += sizeof(UPtr);
                const VkFence fence = *(VkFence*)address;
                address += sizeof(VkFence) * VulkanFrameProcess::kNumMaxSignalFences;

                VkCommandBuffer* pCommandBuffers = reinterpret_cast<VkCommandBuffer*>(data);
                data += sizeof(VkCommandBuffer) * numCommandLists;
                VkPipelineStageFlags* pWaitFlags = reinterpret_cast<VkPipelineStageFlags*>(data);
                data += sizeof(VkPipelineStageFlags) * numCommandLists;
                VkSemaphore* pWaitSemaphores = reinterpret_cast<VkSemaphore*>(data);
                data += sizeof(VkSemaphore) * VulkanFrameProcess::kNumMaxWaitSemaphores;
                VkSemaphore* pSignalSemaphores = reinterpret_cast<VkSemaphore*>(data);
                
                VkSubmitInfo submitInfo = { };
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submitInfo.commandBufferCount = numCommandLists;
                submitInfo.pCommandBuffers = pCommandBuffers;
                submitInfo.pWaitDstStageMask = pWaitFlags;
                submitInfo.waitSemaphoreCount = numWaitSemaphores;
                submitInfo.signalSemaphoreCount = numSignalSemaphores;
                submitInfo.pSignalSemaphores = pSignalSemaphores;
                submitInfo.pWaitSemaphores = pWaitSemaphores;

                VkQueue queue = queryQueue(commandQueueType);
                vkQueueSubmit(queue, 1, &submitInfo, fence);
                break;
            }
            case VulkanFrameProcess::SubmitType_Present:
            {
                VkPresentInfoKHR* info = (VkPresentInfoKHR*)address;
                VkQueue queue = queryQueue(CommandQueueType_Graphics);

                vkQueuePresentKHR(queue, info);
                address += sizeof(VkPresentInfoKHR);
                break;
            }
            case VulkanFrameProcess::SubmitType_Sync:
            default:
                address += 8;
        }
    }
    return RecluseResult_Ok;
}

VkQueue VulkanDevice::queryQueue(CommandQueueType queueType)
{
    switch (queueType)
    {
        case CommandQueueType_Compute: 
            return m_computeQueue;
        case CommandQueueType_Copy:
            return m_copyQueue;
        case CommandQueueType_Graphics:
        default: return m_graphicsQueue;
    }
    return VK_NULL_HANDLE;
}
} // Vulkan
} // RenderApi 
} // Recluse