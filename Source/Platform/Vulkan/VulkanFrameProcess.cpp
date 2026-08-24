//
#include "VulkanFrameProcess.hpp"
#include "VulkanSwapchain.hpp"

#include <functional>

namespace Recluse {
namespace RenderApi {
namespace Vulkan {

VulkanFrameProcess::VulkanFrameProcess(VkDevice device, const VulkanDevice::QueueIndices& queueIndices, const FrameProcess::Description& description)
    : m_maxFramesInFlight(description.maxFramesInFlight)
    , m_currentFrameIndex(0)
    , m_device(device)
    , m_workerPool(description.numCommandListJobThreads)
    , m_queueIndices(queueIndices)
{
    initialize();
}

void VulkanFrameProcess::beginFrame(const FrameDescription& frameDescription)
{
    uint frameIndex = incrementFrameIndex();

    vkWaitForFences(m_device, 1, &m_frames[frameIndex].fence, true, UINT64_MAX);
    vkResetFences(m_device, 1, &m_frames[frameIndex].fence);

    Frame& frame = m_frames[frameIndex];
    frame.frameStream.baseAddress = frame.frameMemory.getBaseAddress();
    frame.frameStream.sizeBytes = 0;
    frame.frameMemory.clear();

    for (auto& it : frame.commandPools)
    {
        VkResult result = vkResetCommandPool(m_device, it.second.pool, 0);
        R_ASSERT(result == VK_SUCCESS);
    }

    if (frameDescription.swapchain)
    {
        VulkanSwapchain* swapchain = dynamic_cast<VulkanSwapchain*>(frameDescription.swapchain);
        swapchain->aquireNextFrameIndex(frame.semaphore);
        m_swapchainRef = swapchain;
    }
}

FrameHandle VulkanFrameProcess::endFrame()
{
    uint frameIndex = currentFrameIndex();
    Frame& frame = m_frames[frameIndex];

    if (m_swapchainRef)
    {
        uint sizeBytes = sizeof(SubmitType) + sizeof(VkSwapchainKHR) 
            + sizeof(uint) + sizeof(VkSemaphore);

        UPtr present = (UPtr)frame.frameMemory.allocateRaw(sizeBytes);
        SubmitType* submitType = (SubmitType*)present;
        *submitType = SubmitType_Present;
        VkSwapchainKHR* swapchain = (VkSwapchainKHR*)(present + sizeof(SubmitType));
        *swapchain = m_swapchainRef->get();
        uint* imageIndex = (uint*)(present + sizeof(SubmitType) + sizeof(VkSwapchainKHR));
        *imageIndex = m_swapchainRef->currentImageIndex();
        VkSemaphore* semaphore = (VkSemaphore*)(present + sizeof(SubmitType) + sizeof(VkSwapchainKHR) + sizeof(uint));
        *semaphore = m_swapchainRef->currentSignalSemaphore();

    }
    FrameHandle handle = reinterpret_cast<FrameHandle>(&frame.frameStream);
    
    m_swapchainRef = nullptr;
    return handle;
}

ResultCode VulkanFrameProcess::submitCommandLists(CommandQueueType type, CommandList* lists, uint numLists)
{
    return RecluseResult_NoImpl;
}

void VulkanFrameProcess::release()
{
    R_ASSERT(m_device);

    m_workerPool.stop();

    for (uint i = 0; i < m_frames.size(); ++i)
    {
        Frame& frame = m_frames[i];
    
        if (frame.fence)
            vkDestroyFence(m_device, frame.fence, nullptr);
        frame.fence = nullptr;

        if (frame.semaphore)
            vkDestroySemaphore(m_device, frame.semaphore, nullptr);
        frame.semaphore = nullptr;

        for (auto& it : frame.commandPools)
        {
            if (!it.second.commandBuffers.empty())
            {
                vkFreeCommandBuffers(m_device, it.second.pool,
                    it.second.commandBuffers.size(), it.second.commandBuffers.data());
            }
            if (!it.second.secondaryCommandBuffers.empty())
            {
                vkFreeCommandBuffers(m_device, it.second.pool,
                    it.second.secondaryCommandBuffers.size(), it.second.secondaryCommandBuffers.data());
            }

            if (it.second.pool)
                vkDestroyCommandPool(m_device, it.second.pool, nullptr);

            it.second.pool = nullptr;
        }
    }
}

void VulkanFrameProcess::initialize()
{
    if (!m_device) return;

    m_frames.resize(m_maxFramesInFlight);

    auto poolCreateFn = [&] (Frame& frame, uint familyIndex) -> void {
        if (familyIndex == VulkanDevice::QueueProperties::kBadIndex)
            return;

        auto it = frame.commandPools.find(familyIndex);

        if (it == frame.commandPools.end())
        {
            CommandPool pool = { };

            VkCommandPoolCreateInfo commandPoolCi = { };
            commandPoolCi.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            commandPoolCi.queueFamilyIndex = familyIndex;
            commandPoolCi.flags = 0;
            VkResult result = vkCreateCommandPool(m_device, &commandPoolCi, nullptr, &pool.pool);
            R_ASSERT(result == VK_SUCCESS);

            frame.commandPools[familyIndex] = pool;
        }
    };
    
    for (uint i = 0; i < m_frames.size(); ++i)
    {
        Frame& frame = m_frames[i];
        poolCreateFn(frame, m_queueIndices.graphics.familyIndex);
        poolCreateFn(frame, m_queueIndices.compute.familyIndex);
        poolCreateFn(frame, m_queueIndices.copy.familyIndex);

        VkFenceCreateInfo fenceCi   = { };
        fenceCi.sType               = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCi.flags               = VK_FENCE_CREATE_SIGNALED_BIT;
        vkCreateFence(m_device, &fenceCi, nullptr, &frame.fence);

        VkSemaphoreCreateInfo semaphoreCi   = { };
        semaphoreCi.sType                   = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        vkCreateSemaphore(m_device, &semaphoreCi, nullptr, &frame.semaphore);
    }
    
    m_workerPool.start();
}

ResultCode VulkanFrameProcess::waitIdle()
{
    VkResult result = vkDeviceWaitIdle(m_device);
    R_ASSERT(result == VK_SUCCESS);
    return result == VK_SUCCESS ? RecluseResult_Ok : RecluseResult_Failed;
}

ResultCode VulkanFrameProcess::waitForFences(Fence* fences, uint numFences)
{
    return RecluseResult_NoImpl;
}

ResultCode VulkanFrameProcess::signalFences(Fence* fences, uint numFences)
{
    return RecluseResult_NoImpl;
}
} // Vulkan
} // RenderApi
} // Recluse