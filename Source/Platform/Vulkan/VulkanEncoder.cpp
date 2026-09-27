//
#include <Recluse/Arch.hpp>
#include "VulkanFrameProcess.hpp"
#include "VulkanSwapchain.hpp"

#include <Shared/CommandOps.hpp>

namespace Recluse {
namespace RenderApi {
namespace Vulkan {

VkResult VulkanFrameProcess::VulkanCommandListEncoder::encode(const CommandStreamChunk& chunk, StateTracker& tracker)
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
                VkCommandBufferInheritanceInfo inheritanceInfo = { };
                beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                beginInfo.flags = chunk.type == Dynamic ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : 0;
                if (chunk.type == Bundle)
                {
                    //beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
                    beginInfo.pInheritanceInfo = &inheritanceInfo;
                    inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
                    inheritanceInfo.renderPass = VK_NULL_HANDLE;
                    inheritanceInfo.subpass = 0;
                    inheritanceInfo.framebuffer = VK_NULL_HANDLE;
                }
                else
                {
                    beginInfo.pInheritanceInfo = nullptr;
                }
                vkBeginCommandBuffer(tracker.commandbuffer, &beginInfo);
                break;
            }
            case CommandOpcode_End:
            {
                // Flush barriers.
                flushBarriers(tracker);
                vkEndCommandBuffer(tracker.commandbuffer);
                break;
            }
            case CommandOpcode_BarrierTransition:
            {
                R_ASSERT_FORMAT(chunk.type == Primary, "Barriers can only be executed on primary command lists! Command list id=%d", chunk.id);
                if (chunk.type == Bundle)
                {
                    R_ERROR("Vulkan", "Bundles can not have barriers! Barriers must be executed on primary command lists!");
                    break;
                }

                BarrierTransitionHeader* transitionHeader = (BarrierTransitionHeader*)(address + sizeof(CommandHeader));
                const uint numTransitions = transitionHeader->numTransitions;
                Transition* transitions = reinterpret_cast<Transition*>(reinterpret_cast<UPtr>(transitionHeader) + sizeof(BarrierTransitionHeader));
                for (uint i = 0; i < numTransitions; ++ i)
                {
                    const Transition& transition = transitions[i];
                    VulkanResource* nativeResource = static_cast<VulkanResource*>(transition.resource);

                    if (tracker.resourceStateDatabase.hasResourceState(transition.resource->getId()) == false)
                    {
                        tracker.resourceStateDatabase.setCurrentResourceState(transition.resource->getId(), ResourceState_Undefined);
                    }

                    const ResourceState currentState = tracker.resourceStateDatabase.queryCurrentResourceState(transition.resource->getId());

                    if (currentState == transition.newState)
                    {
                        // No need to transition, the resource is already in the desired state.
                        continue;
                    }

                    if (nativeResource->isImage())
                    {
                        VkImage image = nativeResource->get<VkImage>();
                        VkImageMemoryBarrier memoryBarrier = { };
                        memoryBarrier.oldLayout = getImageLayout(currentState);
                        memoryBarrier.newLayout = getImageLayout(transition.newState);
                        memoryBarrier.image = image;
                        memoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                        memoryBarrier.srcAccessMask = getDesiredResourceStateAccessMask(currentState);
                        memoryBarrier.dstAccessMask = getDesiredResourceStateAccessMask(transition.newState);
                        memoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;    
                        memoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                        memoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                        memoryBarrier.subresourceRange.baseArrayLayer = 0;
                        memoryBarrier.subresourceRange.baseMipLevel = 0;
                        memoryBarrier.subresourceRange.layerCount = 1;
                        memoryBarrier.subresourceRange.levelCount = 1;

                        VkPipelineStageFlags srcPipelineStage = getDestinationPipelineStage(memoryBarrier.srcAccessMask);
                        VkPipelineStageFlags dstPipelineStage = getDestinationPipelineStage(memoryBarrier.dstAccessMask);

                        barriers[{ srcPipelineStage, dstPipelineStage }].imageBarriers.push_back(memoryBarrier);
                    }
                    else
                    {
                        VkBuffer buffer = nativeResource->get<VkBuffer>();
                        VkBufferMemoryBarrier memoryBarrier = { };
                        memoryBarrier.sType                 = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                        memoryBarrier.srcAccessMask         = getDesiredResourceStateAccessMask(currentState);
                        memoryBarrier.dstAccessMask         = getDesiredResourceStateAccessMask(transition.newState);
                        memoryBarrier.dstQueueFamilyIndex   = VK_QUEUE_FAMILY_IGNORED;
                        memoryBarrier.srcQueueFamilyIndex   = VK_QUEUE_FAMILY_IGNORED;
                        memoryBarrier.offset                = 0;
                        memoryBarrier.size                  = VK_WHOLE_SIZE;
                        memoryBarrier.buffer                = buffer;
                        VkPipelineStageFlags srcPipelineStage = getDestinationPipelineStage(memoryBarrier.srcAccessMask);
                        VkPipelineStageFlags dstPipelineStage = getDestinationPipelineStage(memoryBarrier.dstAccessMask);

                        barriers[{ srcPipelineStage, dstPipelineStage }].bufferBarriers.push_back(memoryBarrier);
                    }

                    // Update the resource state database with the new state.
                    tracker.resourceStateDatabase.setCurrentResourceState(transition.resource->getId(), transition.newState);
                }
                break;
            }
            case CommandOpcode_ExecuteBundles:
            {
                BundlesHeader* bundlesHeader = (BundlesHeader*)(address + sizeof(CommandHeader));
                const uint numBundles = bundlesHeader->numBundles;
                CommandList** bundles = reinterpret_cast<CommandList**>(reinterpret_cast<UPtr>(bundlesHeader) + sizeof(BundlesHeader));

                static VkCommandBuffer bundleBuffers[64] = { };
                for (uint i = 0; i < numBundles; ++i)
                {
                    CommandList* bundleList = bundles[i];
                    VkCommandBuffer secondary = nullptr;
                    do {
                        ScopedLock _lock(tracker.frame.secondaryCommandBufferMutex);
                        auto it = tracker.frame.secondaryCommandBufferMap.find(bundleList->getId());
                        if (it != tracker.frame.secondaryCommandBufferMap.end())
                        {
                            secondary = it->second;
                        }
                    } while (secondary == nullptr);
                    bundleBuffers[i] = secondary;
                }
                vkCmdExecuteCommands(tracker.commandbuffer, numBundles, bundleBuffers);
                break;
            }
            case CommandOpcode_ClearRenderTarget:
            {
                break;
            }
            default:
                break;
        }
        
        address += CommandHeader::packetSizeBytes(header);
    }
    //printf("Encoded command list 0x%08x with %llu bytes.\n", tracker.commandbuffer, (unsigned long long)chunk.sizeBytes);
    return VK_SUCCESS;
}

void VulkanFrameProcess::VulkanCommandListEncoder::flushBarriers(StateTracker& tracker)
{
    for (auto& it : barriers)
    {
        PipelineStage stage = it.first;
        vkCmdPipelineBarrier(tracker.commandbuffer,
            stage.srcStageFlags, stage.dstStageFlags, VK_DEPENDENCY_BY_REGION_BIT, 
            0, nullptr,
             it.second.bufferBarriers.size(), it.second.bufferBarriers.data(), 
            it.second.imageBarriers.size(), it.second.imageBarriers.data());
        it.second.imageBarriers.clear();
        it.second.bufferBarriers.clear();
    }
}
} // Vulkan
} // RenderApi
} // Recluse