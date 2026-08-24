#ifndef RECLUSE_VULKAN_FRAME_PROCESS_HPP
#define RECLUSE_VULKAN_FRAME_PROCESS_HPP

#pragma once

#include <Recluse/Threading/ThreadPool.hpp>
#include <Recluse/RenderApi/Device.hpp>
#include <Recluse/Memory/LinearScratchMemory.hpp>

#include "VulkanCommon.hpp"
#include "VulkanDevice.hpp"

#include <vector>

namespace Recluse {
namespace RenderApi {
namespace Vulkan {

class VulkanSwapchain;

class VulkanFrameProcess : public FrameProcess
{
public:
    struct FrameStream
    {
        UPtr baseAddress;
        uint sizeBytes;
    };

    enum SubmitType 
    {
        SubmitType_CommandBuffers,
        SubmitType_Present,
        SubmitType_Sync,
    };

    VulkanFrameProcess(VkDevice device = VK_NULL_HANDLE, const VulkanDevice::QueueIndices& queueIndices = { }, const FrameProcess::Description& description = { });

    void                        beginFrame(const FrameDescription& frame) override;
    FrameHandle                 endFrame() override;

    ResultCode                  submitCommandLists(CommandQueueType type, CommandList* lists, uint numLists) override;
    ResultCode                  waitForFences(Fence* fences, uint numFences) override;
    ResultCode                  signalFences(Fence* fences, uint numFences) override;
    ResultCode                  waitIdle() override;
    void                        release();
private:

    uint                        incrementFrameIndex() 
        { 
            m_currentFrameIndex = (m_currentFrameIndex + 1) % m_maxFramesInFlight; 
            return m_currentFrameIndex; 
        }

    uint                        currentFrameIndex() const { return m_currentFrameIndex; }

    void                        initialize();

    struct CommandPool
    {
        VkCommandPool pool;
        std::vector<VkCommandBuffer>    commandBuffers;
        std::vector<VkCommandBuffer>    secondaryCommandBuffers;
    };

    struct Frame
    {
        LinearScratchMemory<1024>       frameMemory;
        FrameStream                     frameStream;
        std::map<uint, CommandPool>     commandPools;
        VkFence                         fence;
        VkSemaphore                     semaphore;
    };

    std::vector<Frame>              m_frames;
    uint                            m_currentFrameIndex;
    uint                            m_maxFramesInFlight;
    ThreadPool                      m_workerPool;
    VkDevice                        m_device;
    VulkanDevice::QueueIndices      m_queueIndices;
    VulkanSwapchain*                m_swapchainRef;
};
} // Vulkan
} // RenderApi
} // Recluse
#endif // RECLUSE_VULKAN_FRAME_PROCESS_HPP