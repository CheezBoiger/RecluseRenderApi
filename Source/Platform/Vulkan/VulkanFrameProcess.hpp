#ifndef RECLUSE_VULKAN_FRAME_PROCESS_HPP
#define RECLUSE_VULKAN_FRAME_PROCESS_HPP

#pragma once

#include <Recluse/Threading/ThreadPool.hpp>
#include <Recluse/RenderApi/Device.hpp>
#include <Recluse/Structures/LruCache.hpp>
#include <Recluse/Memory/MemoryArena.hpp>
#include <Recluse/Threading/Threading.hpp>

#include "VulkanCommon.hpp"
#include "VulkanDevice.hpp"
#include "VulkanResource.hpp"

#include <vector>

namespace Recluse {
namespace RenderApi {
namespace Vulkan {

class VulkanSwapchain;

class VulkanFrameProcess : public FrameProcess
{
    class CommandPool;
public:
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

    VulkanFrameProcess(VulkanDevice* device = nullptr, const VulkanDevice::QueueIndices& queueIndices = { }, const FrameProcess::Description& description = { });

    void                        beginFrame(const FrameDescription& frame) override;
    FrameHandle                 endFrame() override;

    ResultCode                  submitCommandLists(CommandQueueType type, CommandList** lists, uint numLists) override;
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
    uint queryFamilyIndex(CommandQueueType type);

    struct CommandPoolContext
    {
        struct CommandPool
        {
            enum PoolType {
                PoolType_Dynamic,
                PoolType_Persistent,
                PoolType_Count,
            };

            VkCommandPool                   pool[PoolType_Count];
            std::unordered_map<CommandList::Id, VkCommandBuffer> persistentCommandBuffers;
            std::vector<VkCommandBuffer> commandbuffers;
            uint currentCbIndex;
            VkCommandBufferLevel level;

            void                        initialize(VkDevice device, VkCommandBufferLevel level, uint familyIndex);
            void                        reset(VkDevice device);
            void                        release(VkDevice device);
            VkCommandBuffer*            obtainCommandBuffers(VkDevice device, 
                                                uint numRequested, uint numOverflowCount);
            VkCommandBuffer             obtainPersistentCommandBuffer(VkDevice device, CommandList::Id id);
            
        };

        CommandPool            primary;
        CommandPool            secondary;


        void initialize(VkDevice device, uint familyIndex);
        void release(VkDevice device);
        void reset(VkDevice device);

        VkCommandBuffer                 obtainCommandBuffer(VkDevice device, 
                                            CommandType type, CommandInstance instance);
        VkCommandBuffer*                obtainCommandBuffers(VkDevice device, 
                                            CommandType type, CommandInstance instance, uint numBuffers);
        
    private:
    };

    struct ThreadContext
    {
        std::map<VulkanDevice::QueueProperties::Index, CommandPoolContext> commandPools;
        void initialize(VkDevice device, const VulkanDevice::QueueIndices& queueIndices);
        void release(VkDevice device);
    };

    struct Frame
    {
        ArenaAllocator<R_KB(4), false, 0>                              scratch;
        ArenaAllocator<R_KB(4), false, 0>                              frameMemory;
        FrameStream                                                     frameStream;
        std::map<U64, ThreadContext>                                    threadContexts;
        VkFence                                                         fence;
        VkSemaphore                                                     frameSemaphore;
        MutexGuard                                                      secondaryCommandBufferMutex;
        std::map<CommandList::Id, VkCommandBuffer>                      secondaryCommandBufferMap;
        void                                                            reset(VkDevice device);
    };

    struct StateTracker
    {
        Frame& frame;
        VkCommandBuffer commandbuffer;
        CommandPoolContext& commandPoolContext;
        ResourceStateDatabase& resourceStateDatabase;
    };

    class VulkanCommandListEncoder
    {
    public:
        VulkanCommandListEncoder(VkDevice device) 
            : m_device(device) { }
        VkResult encode(const CommandStreamChunk& chunk, StateTracker& tracker);
        VkResult operator()(const CommandStreamChunk& chunk, StateTracker& tracker) { return encode(chunk, tracker); }
    private:
        void flushBarriers(StateTracker& tracker);

        VkDevice m_device;

        struct PipelineStage
        {
            VkPipelineStageFlags srcStageFlags;
            VkPipelineStageFlags dstStageFlags;
            
            bool operator==(const PipelineStage& stage) const 
            {
                return (srcStageFlags == stage.srcStageFlags) && (dstStageFlags == stage.dstStageFlags);
            }
        
            bool operator!=(const PipelineStage& stage) const
            {
                return !(*this == stage);
            }

            bool operator()(const PipelineStage& stage) const 
            {
                return (*this == stage);
            }
        };

        struct PipelineStageHasher
        {
            size_t operator()(const PipelineStage& key) const {
                return ((std::hash<uint32_t>()(key.srcStageFlags) ^ (std::hash<uint32_t>()(key.srcStageFlags) << 1)) >> 1);
            }
        };

        struct Barriers
        {
            std::vector<VkImageMemoryBarrier> imageBarriers;
            std::vector<VkBufferMemoryBarrier> bufferBarriers;

            Barriers() : imageBarriers(), bufferBarriers() { }
        };

        std::unordered_map<PipelineStage, Barriers, PipelineStageHasher>   barriers;
    };

    std::vector<Frame>                  m_frames;
    uint                                m_currentFrameIndex;
    uint                                m_maxFramesInFlight;
    ThreadPool                          m_workerPool;
    VulkanDevice*                       m_device;
    VulkanDevice::QueueIndices          m_queueIndices;
    VulkanSwapchain*                    m_swapchainRef;
};
} // Vulkan
} // RenderApi
} // Recluse
#endif // RECLUSE_VULKAN_FRAME_PROCESS_HPP