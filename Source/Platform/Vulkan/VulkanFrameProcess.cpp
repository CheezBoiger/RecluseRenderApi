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
        swapchain->aquireNextFrameIndex(frame.frameSemaphore);
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
            waitSemaphorePtr[0] = frame.frameSemaphore;
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

    // Reset the swapchain link.
    m_swapchainRef = nullptr;

    return handle;
}

ResultCode VulkanFrameProcess::submitCommandLists(CommandQueueType type, CommandList* lists, uint numLists)
{
    if (numLists == 0) return RecluseResult_Ok;

    Frame& frame = m_frames[currentFrameIndex()];
    CommandPool& pool = frame.commandPools[type];
    VulkanCommandListEncoder encoder(m_device);
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
        const CommandStreamChunk* chunks = lists[i].getChunks();
        const uint numChunks = lists[i].getNumChunks();

        for (uint chunkIndex = 0; chunkIndex < numChunks; ++chunkIndex)
        {
            VkCommandBuffer cmdBuffer = pool.obtainCommandBuffer(m_device, chunks[chunkIndex].type, chunks[chunkIndex].instance); 
            encoder(chunks[chunkIndex], cmdBuffer);
            VkCommandBuffer* cmd = (VkCommandBuffer*)packet;
            *cmd = cmdBuffer;
            packet += sizeof(VkCommandBuffer);    
        }
    }

    m_workerPool.waitIdle();

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

        if (frame.frameSemaphore)
            vkDestroySemaphore(m_device, frame.frameSemaphore, nullptr);
        frame.frameSemaphore = nullptr;

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
        vkCreateSemaphore(m_device, &semaphoreCi, nullptr, &frame.frameSemaphore);
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

VkCommandBuffer* VulkanFrameProcess::CommandPool::obtainCommandBuffers(VkDevice device, CommandType type, CommandInstance instance, uint numBuffers)
{
    VkCommandBuffer* result = nullptr;
    if (type == CommandType::Primary)
    {
        if (instance == CommandInstance::Dynamic)
        {
            result = primary.obtainCommandBuffers(device, pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, numBuffers, 2);
        }
    }
    else if (type == CommandType::Bundle)
    {
        if (instance == Dynamic)
        {
            result = secondary.obtainCommandBuffers(device, pool, VK_COMMAND_BUFFER_LEVEL_SECONDARY, numBuffers, 2);
        }
    }
    return result;
}

VkCommandBuffer* VulkanFrameProcess::CommandPool::CommandBufferHandler::obtainCommandBuffers(VkDevice device, VkCommandPool pool, VkCommandBufferLevel level, uint numRequested, uint numOverflowCount)
{
    VkResult result = VK_SUCCESS;
    if (numRequested == 0) return nullptr;

    if ((numRequested + currentCbIndex) >= commandbuffers.size())
    {
        commandbuffers.resize(commandbuffers.size() + numRequested + numOverflowCount);

        VkCommandBufferAllocateInfo info = { };
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        info.commandPool = pool;
        info.commandBufferCount = numRequested + numOverflowCount; // Overflowing buffers also need to be allocated.
        info.level = level;

        result = vkAllocateCommandBuffers(device, &info, &commandbuffers[currentCbIndex]);
    }

    //for (uint i = 0; i < numRequested; ++i)
    //{
    //    out[i] = commandbuffers[currentCbIndex + i];
    //}
    VkCommandBuffer* buffers = nullptr;

    if (result == VK_SUCCESS)
    {
        buffers = &commandbuffers[currentCbIndex];
        currentCbIndex += numRequested;
    }

    return buffers;
}

VkCommandBuffer VulkanFrameProcess::CommandPool::obtainCommandBuffer(VkDevice device, CommandType type, CommandInstance instance)
{
    VkCommandBuffer cmdBuffer = nullptr;
    VkCommandBuffer* result = obtainCommandBuffers(device, type, instance, 1);
    if (result) 
        cmdBuffer = *result;
    return cmdBuffer;
}

void VulkanFrameProcess::CommandPool::CommandBufferHandler::reset()
{
    currentCbIndex = 0;
}

VkResult VulkanFrameProcess::VulkanCommandListEncoder::encode(const CommandStreamChunk& chunk, VkCommandBuffer commandbuffer)
{
    UPtr address = chunk.baseAddress;
    const UPtr endAddress = chunk.baseAddress + chunk.sizeBytes;

    while (address < endAddress)
    {
        CommandHeader* header = reinterpret_cast<CommandHeader*>(address);
        switch (header->opcode)
        {
            case CommandOpcode_Begin:
            {
                VkCommandBufferBeginInfo beginInfo = { };
                beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                beginInfo.flags = chunk.instance == OneTimeOnly ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : 0;
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
                const uint numTransitions = transitionHeader->numTransitions;
                Transition* transitions = reinterpret_cast<Transition*>(reinterpret_cast<UPtr>(transitionHeader) + sizeof(BarrierTransitionHeader));
                for (uint i = 0; i < numTransitions; ++ i)
                {
                    Transition& transition = transitions[i];
                    VulkanResource* nativeResource = static_cast<VulkanResource*>(transition.resource);
                    if (nativeResource->isImage())
                        nativeResource->get<VkImage>();
                    else
                        nativeResource->get<VkBuffer>();
                }
                break;
            }
            default:
                break;
        }
        
        address += CommandHeader::packetSizeBytes(header);
    }

    return VK_SUCCESS;
}
} // Vulkan
} // RenderApi
} // Recluse