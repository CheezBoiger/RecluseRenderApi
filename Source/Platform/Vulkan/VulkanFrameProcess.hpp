#ifndef RECLUSE_VULKAN_FRAME_PROCESS_HPP
#define RECLUSE_VULKAN_FRAME_PROCESS_HPP

#pragma once

#include <Recluse/Threading/ThreadPool.hpp>
#include <Recluse/RenderApi/Device.hpp>
#include <Recluse/Memory/LinearScratchMemory.hpp>
#include <Recluse/Threading/Threading.hpp>

#include "VulkanCommon.hpp"
#include "VulkanDevice.hpp"

#include <vector>

namespace Recluse {
namespace RenderApi {
namespace Vulkan {

class VulkanSwapchain;

class VulkanFrameProcess : public FrameProcess
{
    class CommandPool;
public:
    struct State {
        ResourceState           resourceState;
        VkAccessFlags           accessMask;
        VkPipelineStageFlags    pipelineStage;
    };
    typedef std::unordered_map<VulkanResource::ResourceHandle, State> ResourceStateMap;

    static const uint kNumMaxSignalSemaphores   = 1;
    static const uint kNumMaxWaitSemaphores     = 1;
    static const uint kNumMaxSignalFences       = 1;
    
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

    uint incrementFrameIndex() 
        { 
            m_currentFrameIndex = (m_currentFrameIndex + 1) % m_maxFramesInFlight; 
            return m_currentFrameIndex; 
        }

    uint currentFrameIndex() const { return m_currentFrameIndex; }

    void initialize();

    struct CommandPool
    {
        struct CommandBufferHandler
        {
            std::vector<VkCommandBuffer> commandbuffers;
            std::unordered_map<VkCommandBuffer, ResourceStateMap> localResourceStateMap;
            uint currentCbIndex;

            void                        reset();
            VkCommandBuffer*            obtainCommandBuffers(VkDevice device, VkCommandPool pool, 
                                                VkCommandBufferLevel level, 
                                                uint numRequested, uint numOverflowCount);
            ResourceStateMap*           obtainLocalResourceStateMap(VkCommandBuffer buffer);
        };

        VkCommandPool pool;
        CommandBufferHandler            primary;
        CommandBufferHandler            secondary;

        VkCommandBuffer                 obtainCommandBuffer(VkDevice device, 
                                            CommandType type, CommandInstance instance);
        VkCommandBuffer*                obtainCommandBuffers(VkDevice device, 
                                            CommandType type, CommandInstance instance, uint numBuffers);
        ResourceStateMap*               obtainLocalStateMap(VkCommandBuffer commandbuffer, CommandType type, CommandInstance instance);
        
    private:
    };

    struct StateTracker
    {
        VkCommandBuffer commandbuffer;
        ResourceStateMap* localStateMap;
    };

    class VulkanCommandListEncoder
    {
    public:
        VulkanCommandListEncoder(VkDevice device) 
            : m_device(device)
            , imageBarriers(256)
            , bufferBarriers(256) { }
        VkResult encode(const CommandStreamChunk& chunk, StateTracker& tracker);
        VkResult operator()(const CommandStreamChunk& chunk, StateTracker& tracker) { return encode(chunk, tracker); }
    private:
        VkDevice m_device;
        std::vector<VkImageMemoryBarrier>   imageBarriers;
        std::vector<VkBufferMemoryBarrier>  bufferBarriers;
    };

    struct Frame
    {
        LinearScratchMemory<R_KB(4)>    scratch;
        LinearScratchMemory<R_KB(4)>    frameMemory;
        FrameStream                     frameStream;
        std::map<uint, CommandPool>     commandPools;
        VkFence                         fence;
        VkSemaphore                     frameSemaphore;

        void                            reset(VkDevice device);
    };

    std::vector<Frame>                  m_frames;
    uint                                m_currentFrameIndex;
    uint                                m_maxFramesInFlight;
    ThreadPool                          m_workerPool;
    VkDevice                            m_device;
    VulkanDevice::QueueIndices          m_queueIndices;
    VulkanSwapchain*                    m_swapchainRef;
    ResourceStateMap                    m_resourceStateMap;
};
} // Vulkan
} // RenderApi
} // Recluse
#endif // RECLUSE_VULKAN_FRAME_PROCESS_HPP