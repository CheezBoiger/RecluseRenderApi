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


class VulkanFrameProcess : public FrameProcess
{
public:
    enum SubmitType 
    {
        SubmitType_CommandBuffers,
        SubmitType_Present,
        SubmitType_Sync,
    };

    void                        beginFrame(const FrameDescription& frame) override;
    FrameHandle                 endFrame() override;

private:
    struct BufferFrame
    {
        LinearScratchMemory<1024>    frameMemory;
        VkCommandPool                commandPool;
        std::vector<VkCommandBuffer> commandBuffers;
        std::vector<VkCommandBuffer> secondaryCommandBuffers;
        VkFence                      fence;
    };

    std::vector<BufferFrame> m_bufferFrames;
    ThreadPool m_workerPool;
};
} // Vulkan
} // RenderApi
} // Recluse
#endif // RECLUSE_VULKAN_FRAME_PROCESS_HPP