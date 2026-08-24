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

        it.second.primary.reset();
        it.second.secondary.reset();
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
        const uint submitSizeBytes = sizeof(SubmitType) + sizeof(VkPresentInfoKHR);
        const uint scratchSizeBytes = sizeof(VkSwapchainKHR) 
            + sizeof(uint) + sizeof(VkSemaphore);

        UPtr present = (UPtr)frame.frameMemory.allocateRaw(submitSizeBytes);
        SubmitType* submitType = (SubmitType*)present;
        *submitType = SubmitType_Present;
        VkPresentInfoKHR* info = (VkPresentInfoKHR*)(present + sizeof(SubmitType));

        UPtr scratchAddress = (UPtr)frame.scratch.allocateRaw(scratchSizeBytes);
        VkSwapchainKHR* swapchain = (VkSwapchainKHR*)scratchAddress;
        *swapchain = m_swapchainRef->get();
        uint* imageIndex = (uint*)(scratchAddress + sizeof(VkSwapchainKHR));
        *imageIndex = m_swapchainRef->currentImageIndex();
        VkSemaphore* semaphore = (VkSemaphore*)(scratchAddress + sizeof(VkSwapchainKHR) + sizeof(uint));
        *semaphore = m_swapchainRef->currentSignalSemaphore();
    
        *info = { };
        info->sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        info->pImageIndices = imageIndex;
        info->swapchainCount = 1;
        info->pSwapchains = swapchain;
        info->pWaitSemaphores = semaphore;
        info->waitSemaphoreCount = 1;

        frame.frameStream.sizeBytes += submitSizeBytes;
    }
    FrameHandle handle = reinterpret_cast<FrameHandle>(&frame.frameStream);
    
    m_swapchainRef = nullptr;
    return handle;
}

ResultCode VulkanFrameProcess::submitCommandLists(CommandQueueType type, CommandList* lists, uint numLists)
{
    if (numLists == 0) return RecluseResult_Ok;

    VulkanCommandListEncoder encoder(this);

    Frame& frame = m_frames[currentFrameIndex()];
    CommandPool& pool = frame.commandPools[type];
    ResultCode result = RecluseResult_Ok;

    const uint numMaxSignalSemaphores = 1;
    const uint numMaxWaitSemaphores = 1;
    const uint numMaxSignalFences = 1;
    
    const uint submitBytes = sizeof(SubmitType) + sizeof(VkSubmitInfo) + sizeof(CommandQueueType) + sizeof(VkFence) * numMaxSignalFences;
    const uint scratchSizeBytes = sizeof(VkCommandBuffer) * numLists + 
        sizeof(VkPipelineStageFlags) * numLists +
        sizeof(VkSemaphore) * numMaxWaitSemaphores + sizeof(VkSemaphore) * numMaxSignalSemaphores;
    
    UPtr packet = (UPtr)frame.frameMemory.allocateRaw(submitBytes);
    SubmitType* submitType = reinterpret_cast<SubmitType*>(packet);
    *submitType = SubmitType_CommandBuffers;

    packet += sizeof(SubmitType);

    VkSubmitInfo* info = reinterpret_cast<VkSubmitInfo*>(packet);
    info->sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    info->pNext = nullptr;

    packet += sizeof(VkSubmitInfo);

    CommandQueueType* queueType = reinterpret_cast<CommandQueueType*>(packet);
    *queueType = type;

    packet += sizeof(CommandQueueType);
    
    VkFence* fence = (VkFence*)packet;
    *fence = frame.fence;

    packet += sizeof(VkFence);

    // scratch data allocation.

    packet = (UPtr)frame.scratch.allocateRaw(scratchSizeBytes);
    const UPtr commandBufferStartAddress = packet;

    for (uint i = 0; i < numLists ; ++i)
    {
        R_ASSERT(lists[i].getType() == CommandList::CommandType::Primary);
        VkCommandBuffer cmdBuffer = pool.obtainCommandBuffer(m_device, lists[i].getType(), lists[i].getInstance());
        result = encoder(lists[i].getChunk(), cmdBuffer);
        VkCommandBuffer* cmd = (VkCommandBuffer*)packet;
        *cmd = cmdBuffer;

        packet += sizeof(VkCommandBuffer);    
    }

    const UPtr waitStageFlagsStartAddress = packet;
    
    for (uint i = 0; i < numLists; ++i)
    {
        VkPipelineStageFlags* flags = (VkPipelineStageFlags*)packet;
        *flags = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        packet += sizeof(VkPipelineStageFlags);
    }

    UPtr waitSemaphoreStartAddress = packet;
    UPtr signalSemaphoreStartAddress = packet + sizeof(VkSemaphore) * numMaxWaitSemaphores; 

    // Now fill in the VkSubmitInfo

    info->commandBufferCount = numLists;
    info->pCommandBuffers = (VkCommandBuffer*)commandBufferStartAddress;
    info->signalSemaphoreCount = 0;
    info->waitSemaphoreCount = 0;
    info->pSignalSemaphores = (VkSemaphore*)signalSemaphoreStartAddress;
    info->pWaitSemaphores = (VkSemaphore*)waitSemaphoreStartAddress;
    info->pWaitDstStageMask = (VkPipelineStageFlags*)waitStageFlagsStartAddress;

    frame.frameStream.sizeBytes += submitBytes;
    return result;
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
            if (!it.second.primary.commandbuffers.empty())
            {
                vkFreeCommandBuffers(m_device, it.second.pool,
                    it.second.primary.commandbuffers.size(), it.second.primary.commandbuffers.data());
            }
            if (!it.second.secondary.commandbuffers.empty())
            {
                vkFreeCommandBuffers(m_device, it.second.pool,
                    it.second.secondary.commandbuffers.size(), it.second.secondary.commandbuffers.data());
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

VkResult VulkanFrameProcess::CommandPool::obtainCommandBuffers(VkDevice device, CommandList::CommandType type, CommandList::CommandInstance instance, uint numBuffers, VkCommandBuffer* out)
{
    VkResult result = VK_SUCCESS;
    if (type == CommandList::CommandType::Primary)
    {
        if (instance == CommandList::CommandInstance::Dynamic)
        {
            result = primary.obtainCommandBuffers(device, pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, out, numBuffers, 2);
        }
    }
    else if (type == CommandList::CommandType::Bundle)
    {
        if (instance == CommandList::CommandInstance::Dynamic)
        {
            result = secondary.obtainCommandBuffers(device, pool, VK_COMMAND_BUFFER_LEVEL_SECONDARY, out, numBuffers, 2);
        }
    }
    return result;
}

VkResult VulkanFrameProcess::CommandPool::CommandBufferHandler::obtainCommandBuffers(VkDevice device, VkCommandPool pool, VkCommandBufferLevel level, VkCommandBuffer* out, uint numRequested, uint numOverflowCount)
{
    VkResult result = VK_SUCCESS;
    if (numRequested == 0) return result;

    if ((numRequested + currentCbIndex) >= commandbuffers.size())
    {
        commandbuffers.resize(commandbuffers.size() + numRequested + numOverflowCount);

        VkCommandBufferAllocateInfo info = { };
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        info.commandPool = pool;
        info.commandBufferCount = numRequested;
        info.level = level;

        result = vkAllocateCommandBuffers(device, &info, &commandbuffers[currentCbIndex]);
    }

    for (uint i = 0; i < numRequested; ++i)
    {
        out[i] = commandbuffers[currentCbIndex + i];
    }

    if (result == VK_SUCCESS)
        currentCbIndex += numRequested;

    return result;
}

VkCommandBuffer VulkanFrameProcess::CommandPool::obtainCommandBuffer(VkDevice device, CommandList::CommandType type, CommandList::CommandInstance instance)
{
    VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;
    VkResult result = obtainCommandBuffers(device, type, instance, 1, &cmdBuffer);
    R_ASSERT(result == VK_SUCCESS);
    return cmdBuffer;
}

void VulkanFrameProcess::CommandPool::CommandBufferHandler::reset()
{
    currentCbIndex = 0;
}

ResultCode VulkanFrameProcess::VulkanCommandListEncoder::encode(const CommandStreamChunk& chunk, VkCommandBuffer commandbuffer)
{
    return RecluseResult_NoImpl;
}
} // Vulkan
} // RenderApi
} // Recluse