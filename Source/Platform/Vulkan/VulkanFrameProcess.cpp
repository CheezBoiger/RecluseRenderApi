//
#include "VulkanFrameProcess.hpp"

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

    vkWaitForFences(m_device, 1, &m_bufferFrames[frameIndex].fence, true, UINT64_MAX);
    vkResetFences(m_device, 1, &m_bufferFrames[frameIndex].fence);

    m_bufferFrames[frameIndex].frameStream.baseAddress = m_bufferFrames[frameIndex].frameMemory.getBaseAddress();
    m_bufferFrames[frameIndex].frameStream.sizeBytes = 0;
}

FrameHandle VulkanFrameProcess::endFrame()
{
    uint frameIndex = currentFrameIndex();
    BufferFrame& frame = m_bufferFrames[frameIndex];
    FrameHandle handle = reinterpret_cast<FrameHandle>(&frame.frameStream);
    return handle;
}

ResultCode VulkanFrameProcess::submitCommandLists(CommandQueueType type, CommandList* lists, uint numLists)
{
    return RecluseResult_NoImpl;
}

void VulkanFrameProcess::release()
{
    for (uint i = 0; i < m_bufferFrames.size(); ++i)
    {
        
    }
}

void VulkanFrameProcess::initialize()
{
    if (!m_device) return;

    m_bufferFrames.resize(m_maxFramesInFlight);

    auto poolCreateFn = [&] (BufferFrame& frame, uint familyIndex) -> void {
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
    
    for (uint i = 0; i < m_bufferFrames.size(); ++i)
    {
        BufferFrame& frame = m_bufferFrames[i];
        poolCreateFn(frame, m_queueIndices.graphics.familyIndex);
        poolCreateFn(frame, m_queueIndices.compute.familyIndex);
        poolCreateFn(frame, m_queueIndices.copy.familyIndex);
    }
    
}
} // Vulkan
} // RenderApi
} // Recluse