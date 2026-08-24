//
#include "VulkanFrameProcess.hpp"
#include "VulkanSwapchain.hpp"

#include <Shared/CommandOps.hpp>

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
    frame.scratch.clear();

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

    UPtr addr = frame.frameStream.baseAddress;
    const UPtr endAddr = addr + frame.frameStream.sizeBytes;
    if (addr < endAddr)
    {
        // TODO: We should definitely make structures for these, as the pointer logic is atrocious.
        VkFence* fence = reinterpret_cast<VkFence*>(addr + sizeof(SubmitType) + sizeof(VkSubmitInfo) + sizeof(CommandQueueType) + sizeof(UPtr));
        *fence = frame.fence;

        if (m_swapchainRef)
        {
            VkSubmitInfo* info = reinterpret_cast<VkSubmitInfo*>(addr + sizeof(SubmitType));
            UPtr data = *reinterpret_cast<UPtr*>(addr + sizeof(SubmitType) + sizeof(VkSubmitInfo) + sizeof(CommandQueueType));
            VkSemaphore* waitSemaphorePtr = reinterpret_cast<VkSemaphore*>(data + sizeof(VkCommandBuffer) * info->commandBufferCount + 
                sizeof(VkPipelineStageFlags) * info->commandBufferCount);
            waitSemaphorePtr[0] = frame.semaphore;
            info->waitSemaphoreCount = 1;
        }
    }

    if (m_swapchainRef)
    {
        if (frame.frameStream.sizeBytes != 0)
        {
            while (addr < endAddr)
            {
                const uint sizeBytes = sizeof(SubmitType) + sizeof(VkSubmitInfo) + sizeof(CommandQueueType) + sizeof(UPtr) + sizeof(VkFence) * kNumMaxSignalFences;
                if ((addr + sizeBytes) >= endAddr)
                    break;
                addr += sizeBytes;
            }

            // TODO: We should definitely make structures for these, as the pointer logic is atrocious.
            VkSubmitInfo* info = reinterpret_cast<VkSubmitInfo*>(addr + sizeof(SubmitType));
            UPtr data = *reinterpret_cast<UPtr*>(addr + sizeof(SubmitType) + sizeof(VkSubmitInfo) + sizeof(CommandQueueType));
            VkSemaphore* waitSemaphorePtr = reinterpret_cast<VkSemaphore*>(data + sizeof(VkCommandBuffer) * info->commandBufferCount + 
                sizeof(VkPipelineStageFlags) * info->commandBufferCount);
            VkSemaphore* signalSemaphorePtr = reinterpret_cast<VkSemaphore*>(data + sizeof(VkCommandBuffer) * info->commandBufferCount + 
                sizeof(VkPipelineStageFlags) * info->commandBufferCount + sizeof(VkSemaphore) * kNumMaxWaitSemaphores);
            signalSemaphorePtr[0] = m_swapchainRef->currentSignalSemaphore();
            info->signalSemaphoreCount = 1;
        }

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

    Frame& frame = m_frames[currentFrameIndex()];
    CommandPool& pool = frame.commandPools[type];
    VulkanCommandListEncoder encoder(m_device, pool);
    ResultCode result = RecluseResult_Ok;
    
    const uint submitBytes = sizeof(SubmitType) + sizeof(VkSubmitInfo) + sizeof(CommandQueueType) + sizeof(UPtr) + sizeof(VkFence) * kNumMaxSignalFences;
    const uint scratchSizeBytes = sizeof(VkCommandBuffer) * numLists + 
        sizeof(VkPipelineStageFlags) * numLists +
        sizeof(VkSemaphore) * kNumMaxWaitSemaphores + sizeof(VkSemaphore) * kNumMaxSignalSemaphores;
    
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
    UPtr* dataAddress = (UPtr*)packet;
    packet += sizeof(UPtr);
    
    VkFence* fence = (VkFence*)packet;
    packet += sizeof(VkFence);

    // scratch data allocation.

    packet = (UPtr)frame.scratch.allocateRaw(scratchSizeBytes);
    *dataAddress = packet;
    const UPtr commandBufferStartAddress = packet;

    for (uint i = 0; i < numLists ; ++i)
    {
        VkCommandBuffer cmdBuffer = encoder(lists[i].getChunk());
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
    UPtr signalSemaphoreStartAddress = packet + sizeof(VkSemaphore) * kNumMaxWaitSemaphores; 

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
    m_workerPool.waitIdle();
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

VkCommandBuffer VulkanFrameProcess::VulkanCommandListEncoder::encode(const CommandStreamChunk& chunk)
{
    UPtr address = chunk.baseAddress;
    const UPtr endAddress = chunk.baseAddress + chunk.sizeBytes;
    VkCommandBuffer commandbuffer = VK_NULL_HANDLE;

    while (address < endAddress)
    {
        CommandHeader* header = reinterpret_cast<CommandHeader*>(address);
        switch (header->opcode)
        {
            case CommandOpcode_Begin:
            {
                CommandListDescription* description = (CommandListDescription*)(address + sizeof(CommandHeader));
                commandbuffer = m_pool.obtainCommandBuffer(m_device, description->type, description->instance);

                VkCommandBufferBeginInfo beginInfo = { };
                beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                beginInfo.flags = description->instance == CommandList::OneTimeOnly ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : 0;
                vkBeginCommandBuffer(commandbuffer, &beginInfo);
                break;
            }
            case CommandOpcode_End:
            {
                vkEndCommandBuffer(commandbuffer);
                break;
            }
            case CommandOpcode_BarrierTransition:
            {
                BarrierTransitionHeader* transitionHeader = (BarrierTransitionHeader*)(address + sizeof(CommandHeader));
                break;
            }
            default:
                break;
        }
        
        address += CommandHeader::packetSizeBytes(header);
    }

    return commandbuffer;
}
} // Vulkan
} // RenderApi
} // Recluse