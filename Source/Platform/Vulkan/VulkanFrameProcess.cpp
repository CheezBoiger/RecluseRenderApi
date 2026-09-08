//
#include "VulkanFrameProcess.hpp"
#include "VulkanSwapchain.hpp"

#include <Shared/CommandOps.hpp>

#include <Recluse/Utility.hpp>
#include <functional>
#include <chrono>
#include <thread>

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
    frame.reset(m_device);

    if (frameDescription.swapchain)
    {
        VulkanSwapchain* swapchain = dynamic_cast<VulkanSwapchain*>(frameDescription.swapchain);
        swapchain->aquireNextFrameIndex(frame.frameSemaphore);
        m_swapchainRef = swapchain;
    }
}

void VulkanFrameProcess::CommandPool::reset(VkDevice device)
{
    R_ASSERT(device != VK_NULL_HANDLE);
    R_ASSERT(pool != VK_NULL_HANDLE);
    VkResult result = vkResetCommandPool(device, pool, 0);
    R_ASSERT(result == VK_SUCCESS);
    primary.reset();
    secondary.reset();
}

void VulkanFrameProcess::Frame::reset(VkDevice device)
{
    frameStream.baseAddress = frameMemory.getBaseAddress();
    frameStream.sizeBytes   = 0;

    frameMemory.clear();
    scratch.clear();
    secondaryCommandBufferMap.clear();

    for (auto& it : threadContexts)
    {
        for (auto& poolIt : it.second.commandPools)
        {
            poolIt.second.reset(device);
        }
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
        VkFence* fence  = reinterpret_cast<VkFence*>(addr + sizeof(SubmitType) + sizeof(uint) * 3 + sizeof(CommandQueueType) + sizeof(UPtr));
        *fence          = frame.fence;

        if (m_swapchainRef)
        {
            uint* waitSemaphoreCount        = reinterpret_cast<uint*>(addr + sizeof(SubmitType) + sizeof(uint));
            const uint commandBufferCount   = *reinterpret_cast<uint*>(addr + sizeof(SubmitType));
            UPtr data                       = *reinterpret_cast<UPtr*>(addr + sizeof(SubmitType) + sizeof(uint) * 3 + sizeof(CommandQueueType));
            VkSemaphore* waitSemaphorePtr   = reinterpret_cast<VkSemaphore*>(data + sizeof(VkCommandBuffer) * commandBufferCount + 
                                                sizeof(VkPipelineStageFlags) * commandBufferCount);
            waitSemaphorePtr[0]             = frame.frameSemaphore;
            *waitSemaphoreCount             += 1;
        }
    }

    if (m_swapchainRef)
    {
        if (frame.frameStream.sizeBytes != 0)
        {
            while (addr < endAddr)
            {
                const uint sizeBytes = sizeof(SubmitType) + sizeof(uint) * 3 + sizeof(CommandQueueType) + sizeof(UPtr) + sizeof(VkFence) * kNumMaxSignalFences;
                if ((addr + sizeBytes) >= endAddr)
                    break;
                addr += sizeBytes;
            }
            uint* numSignalSemaphores           = reinterpret_cast<uint*>(addr + sizeof(SubmitType) + sizeof(uint) * 2);
            const uint commandBufferCount       = *reinterpret_cast<uint*>(addr + sizeof(SubmitType));
            // TODO: We should definitely make structures for these, as the pointer logic is atrocious.
            UPtr data                           = *reinterpret_cast<UPtr*>(addr + sizeof(SubmitType) + sizeof(uint) * 3 + sizeof(CommandQueueType));
            VkSemaphore* waitSemaphorePtr       = reinterpret_cast<VkSemaphore*>(data + sizeof(VkCommandBuffer) * commandBufferCount + 
                                                    sizeof(VkPipelineStageFlags) * commandBufferCount);
            VkSemaphore* signalSemaphorePtr     = reinterpret_cast<VkSemaphore*>(data + sizeof(VkCommandBuffer) * commandBufferCount + 
                                                    sizeof(VkPipelineStageFlags) * commandBufferCount + sizeof(VkSemaphore) * kNumMaxWaitSemaphores);
            signalSemaphorePtr[0]               = m_swapchainRef->currentSignalSemaphore();
            *numSignalSemaphores                += 1;
        }

        const uint submitSizeBytes  = sizeof(SubmitType) + sizeof(VkPresentInfoKHR);
        const uint scratchSizeBytes = sizeof(VkSwapchainKHR) + sizeof(uint) + sizeof(VkSemaphore);

        UPtr present                = (UPtr)frame.frameMemory.allocateRaw(submitSizeBytes);
        SubmitType* submitType      = (SubmitType*)present;
        *submitType                 = SubmitType_Present;
        VkPresentInfoKHR* info      = (VkPresentInfoKHR*)(present + sizeof(SubmitType));

        UPtr scratchAddress         = (UPtr)frame.scratch.allocateRaw(scratchSizeBytes);
        VkSwapchainKHR* swapchain   = (VkSwapchainKHR*)scratchAddress;
        *swapchain                  = m_swapchainRef->get();
        uint* imageIndex            = (uint*)(scratchAddress + sizeof(VkSwapchainKHR));
        *imageIndex                 = m_swapchainRef->currentImageIndex();
        VkSemaphore* semaphore      = (VkSemaphore*)(scratchAddress + sizeof(VkSwapchainKHR) + sizeof(uint));
        *semaphore                  = m_swapchainRef->currentSignalSemaphore();
    
        *info                       = { };
        info->sType                 = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        info->pImageIndices         = imageIndex;
        info->swapchainCount        = 1;
        info->pSwapchains           = swapchain;
        info->pWaitSemaphores       = semaphore;
        info->waitSemaphoreCount    = 1;

        frame.frameStream.sizeBytes += submitSizeBytes;
    }

    FrameHandle handle = reinterpret_cast<FrameHandle>(&frame.frameStream);

    // Reset the swapchain link.
    m_swapchainRef = nullptr;

    // Wait for the thread pool to finish encoding all command lists.
    m_workerPool.waitIdle();

    return handle;
}

ResultCode VulkanFrameProcess::submitCommandLists(CommandQueueType type, CommandList* lists, uint numLists)
{
    if (numLists == 0) return RecluseResult_Ok;

    Frame& frame = m_frames[currentFrameIndex()];
    ResultCode result = RecluseResult_Ok;

    uint totalSecondaryCommandBufferCount = 0;
    for (uint i = 0; i < numLists; ++i)
    {
        totalSecondaryCommandBufferCount += lists[i].getNumChunks();
    }

    struct Submittal {
        SubmitType          type;
        CommandQueueType    queueType;
        uint                numCommandLists;
        uint                numWaitSemaphores;
        uint                numSignalSemaphores;
        UPtr                data;
        VkFence             fence;
    };
    
    const uint submitBytes = sizeof(SubmitType) + sizeof(uint) * 3 + sizeof(CommandQueueType) + sizeof(UPtr) + sizeof(VkFence) * kNumMaxSignalFences;
    const uint scratchSizeBytes = sizeof(VkCommandBuffer) * numLists + 
        sizeof(VkPipelineStageFlags) * numLists +
        sizeof(VkSemaphore) * kNumMaxWaitSemaphores + sizeof(VkSemaphore) * kNumMaxSignalSemaphores;

    // scratch data allocation.

    PacketBuilder dataPacket(frame.scratch.allocateRaw(scratchSizeBytes));
    UPtr startDataAddress = dataPacket.raw();

    auto func = [&] (Frame& frame, uint familyIndex, VkCommandBuffer* commandbufferOut, CommandStreamChunk chunk) -> void {
        ThreadContext& threadContext = frame.threadContexts[getCurrentThreadId()];
        CommandPool& commandPool = threadContext.commandPools[familyIndex];
        VkCommandBuffer cmdBuffer = commandPool.obtainCommandBuffer(m_device, chunk.type, chunk.instance);  

        VulkanCommandListEncoder encoder(m_device);
        StateTracker tracker = { frame, cmdBuffer, commandPool };
        encoder(chunk, tracker);

        if (commandbufferOut)
            *commandbufferOut = cmdBuffer;

        if (chunk.type == CommandType::Bundle)
        {                
            // If it is a bundle, we need to store the command buffer in the frame's secondary command buffer map.
            ScopedLock _lock(frame.secondaryCommandBufferMutex);
            auto it = frame.secondaryCommandBufferMap.find(chunk.id);
            if (it == frame.secondaryCommandBufferMap.end())
            {
                frame.secondaryCommandBufferMap[chunk.id] = cmdBuffer;
            }
        }  
    };

    for (uint i = 0; i < numLists ; ++i)
    {
        const CommandStreamChunk* chunks    = lists[i].getChunks();
        const uint numChunks                = lists[i].getNumChunks();

        // Submit each bundle chunk to the thread pool for encoding. This should be submitted first, as the primary command list
        // will be recording after all bundles are encoded.
        for (uint chunkIndex = 0; chunkIndex < numChunks; ++chunkIndex)
        {
            m_workerPool.submitTask(func, std::ref(frame), queryFamilyIndex(type), nullptr, chunks[chunkIndex]);
        }

        VkCommandBuffer* out = dataPacket.write<VkCommandBuffer>(nullptr);
        m_workerPool.submitTask(func, std::ref(frame), queryFamilyIndex(type), out, lists[i].getPrimaryChunk());
    }

    for (uint i = 0; i < numLists; ++i)
    {
        const CommandStreamChunk& chunk = lists[i].getPrimaryChunk();
        if (chunk.type == CommandType::Primary)
            dataPacket.write<VkPipelineStageFlags>(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
    }

    PacketBuilder packet(frame.frameMemory.allocateRaw(submitBytes));

    SubmitType* submitType          = packet.write(SubmitType_CommandBuffers);
    uint* submitNumCommandLists     = packet.write(numLists);
    uint* submitWaitSemaphores      = packet.write(0u);
    uint* submitSignalSemaphores    = packet.write(0u);
    CommandQueueType* queueType     = packet.write(type);
    UPtr* dataAddress               = packet.write<UPtr>(startDataAddress);
    VkFence* fence                  = packet.write<VkFence>(VK_NULL_HANDLE);

    frame.frameStream.sizeBytes += submitBytes;

    return result;
}

uint VulkanFrameProcess::queryFamilyIndex(CommandQueueType type)
{
    uint familyIndex = 0;
    switch (type)
    {
        case CommandQueueType_Copy:
            familyIndex = m_queueIndices.copy.familyIndex;
            break;
        case CommandQueueType_Compute:
            familyIndex = m_queueIndices.compute.familyIndex;
            break;
        case CommandQueueType_Graphics:
        default:
            familyIndex = m_queueIndices.graphics.familyIndex;
            break;
    };
    return familyIndex;
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

        for (auto& it : frame.threadContexts)
        {
            it.second.release(m_device);
        }
    }
}

void VulkanFrameProcess::CommandPool::initialize(VkDevice device, uint familyIndex)
{
    VkCommandPoolCreateInfo commandPoolCi = { };
    commandPoolCi.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCi.queueFamilyIndex = familyIndex;
    commandPoolCi.flags = 0;
    VkResult result = vkCreateCommandPool(device, &commandPoolCi, nullptr, &pool);
    R_ASSERT(result == VK_SUCCESS);
}

void VulkanFrameProcess::CommandPool::release(VkDevice device)
{
    if (!device) return;
    if (!primary.commandbuffers.empty())
    {
        vkFreeCommandBuffers(device, pool,
            primary.commandbuffers.size(), primary.commandbuffers.data());
    }

    if (!secondary.commandbuffers.empty())
    {
        vkFreeCommandBuffers(device, pool,
            secondary.commandbuffers.size(), secondary.commandbuffers.data());
    }

    if (pool)
        vkDestroyCommandPool(device, pool, nullptr);

    pool = nullptr;
}

void VulkanFrameProcess::initialize()
{
    if (!m_device) return;
    
    m_workerPool.start();

    m_frames.resize(m_maxFramesInFlight);
    
    for (uint i = 0; i < m_frames.size(); ++i)
    {
        Frame& frame = m_frames[i];

        VkFenceCreateInfo fenceCi   = { };
        fenceCi.sType               = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCi.flags               = VK_FENCE_CREATE_SIGNALED_BIT;
        vkCreateFence(m_device, &fenceCi, nullptr, &frame.fence);

        VkSemaphoreCreateInfo semaphoreCi   = { };
        semaphoreCi.sType                   = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        vkCreateSemaphore(m_device, &semaphoreCi, nullptr, &frame.frameSemaphore);

        for (uint i = 0; i < m_workerPool.getWorkerCount(); ++i)
        {
            ThreadContext& threadContext = frame.threadContexts[m_workerPool.getWorkerId(i)];
            threadContext.initialize(m_device, m_queueIndices);
        }
    }
}

ResultCode VulkanFrameProcess::waitIdle()
{
    m_workerPool.waitIdle();
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

void VulkanFrameProcess::ThreadContext::initialize(VkDevice device, const VulkanDevice::QueueIndices& queueIndices)
{
    auto createPoolFn = [&](uint familyIndex) -> void {
        if (familyIndex == VulkanDevice::QueueProperties::kBadIndex)
            return;
        auto it = commandPools.find(familyIndex);
        if (it == commandPools.end())
        {
            CommandPool pool = { };
            pool.initialize(device, familyIndex);
            commandPools[familyIndex] = pool;
        }
    };

    createPoolFn(queueIndices.graphics.familyIndex);
    createPoolFn(queueIndices.compute.familyIndex);
    createPoolFn(queueIndices.copy.familyIndex);
}

void VulkanFrameProcess::ThreadContext::release(VkDevice device)
{
    for (auto& it : commandPools)
    {
        it.second.release(device);
    }
    commandPools.clear();
}
} // Vulkan
} // RenderApi
} // Recluse