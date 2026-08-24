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
    static const uint kNumMaxSignalSemaphores = 1;
    static const uint kNumMaxWaitSemaphores = 1;
    static const uint kNumMaxSignalFences = 1;
    
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

    class VulkanCommandListEncoder
    {
    public:
        VulkanCommandListEncoder(VulkanFrameProcess* process) : m_process(process) { }
        ResultCode encode(const CommandStreamChunk& chunk, VkCommandBuffer cmdBuffer);

        ResultCode operator()(const CommandStreamChunk& chunk, VkCommandBuffer buffer) { return encode(chunk, buffer); }
    private:
        VulkanFrameProcess* m_process;
    };
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
        struct CommandBufferHandler
        {
            std::vector<VkCommandBuffer> commandbuffers;
            uint currentCbIndex;

            void                       reset();
            VkResult                   obtainCommandBuffers(VkDevice device, VkCommandPool pool, 
                                                VkCommandBufferLevel level, VkCommandBuffer* out, 
                                                uint numRequested, uint numOverflowCount);
        };

        VkCommandPool pool;
        CommandBufferHandler            primary;
        CommandBufferHandler            secondary;

        VkCommandBuffer                 obtainCommandBuffer(VkDevice device, 
                                            CommandList::CommandType type, CommandList::CommandInstance instance);
        VkResult                        obtainCommandBuffers(VkDevice device, 
                                            CommandList::CommandType type, CommandList::CommandInstance instance, uint numBuffers,
                                            VkCommandBuffer* out);
    private:
    };

    struct Frame
    {
        LinearScratchMemory<1024>       scratch;
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