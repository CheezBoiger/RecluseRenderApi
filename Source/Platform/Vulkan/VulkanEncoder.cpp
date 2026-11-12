//
#include <Recluse/Arch.hpp>
#include <Recluse/Math/MathCommons.hpp>

#include "VulkanFrameProcess.hpp"
#include "VulkanSwapchain.hpp"

#include <Shared/CommandOps.hpp>

#include <array>

namespace Recluse {
namespace RenderApi {
namespace Vulkan {

typedef void (*CommandEncodeFn)(VulkanCommandListEncoder&, const CommandStreamChunk&, StateTracker&);

std::array<CommandEncodeFn, CommandOpcode_Count> kInstructionTable = {
    nullptr
};

VkResult VulkanCommandListEncoder::encode(const CommandStreamChunk& chunk, StateTracker& tracker)
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
                VkCommandBufferInheritanceRenderingInfo renderingInfo = { };
                VkFormat formats[8] = { };

                beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                beginInfo.flags = chunk.type == Dynamic ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : 0;
                if (chunk.type == Bundle)
                {
                    if (chunk.flags & CommandStreamChunk::Flag_InRenderPass)
                    {
                        beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
                        inheritanceInfo.pNext = &renderingInfo;
                        renderingInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_RENDERING_INFO;
                        BindRenderTargetsHeader* rtHeader = reinterpret_cast<BindRenderTargetsHeader*>(chunk.inheritanceDataAddress);
                        renderingInfo.colorAttachmentCount = rtHeader->numRenderTargets;
                        renderingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

                        ResourceView* views = reinterpret_cast<ResourceView*>(reinterpret_cast<UPtr>(rtHeader) + sizeof(BindRenderTargetsHeader));
                        for (uint i = 0; i < rtHeader->numRenderTargets; ++i)
                        {
                            ResourceView view = views[i];
                            Descriptor* descriptor = reinterpret_cast<Descriptor*>(view);
                            formats[i] = Vulkan::getVulkanFormat(descriptor->format);
                        }

                        renderingInfo.pColorAttachmentFormats = formats;
                        if (rtHeader->hasDepthStencil)
                        {
                            ResourceView depthStencilView = views[rtHeader->numRenderTargets];
                            Descriptor* descriptor = reinterpret_cast<Descriptor*>(depthStencilView);
                            renderingInfo.depthAttachmentFormat = Vulkan::getVulkanFormat(descriptor->format);
                            renderingInfo.stencilAttachmentFormat = Vulkan::getVulkanFormat(descriptor->format);
                        }
                    }

                    beginInfo.pInheritanceInfo = &inheritanceInfo;
                    inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
                    // We need to inherit the render pass and framebuffer if we are in a render pass.
                    // Since we use dynamic rendering, we can just set the render pass and framebuffer to null, 
                    // but we will still need to provide the formats of the render targets and depth stencil if we are in a render pass.
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
                // End the render pass if we are in one.
                endRenderPass(tracker);
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
                    R_ASSERT_FORMAT(transition.oldState != transition.newState, "Transition states are the same! Old state: %d, New state: %d, ignoring...", transition.oldState, transition.newState);
                    if (transition.oldState == transition.newState) continue;

                    VulkanResource* nativeResource = static_cast<VulkanResource*>(transition.resource);

                    if (nativeResource->isImage())
                    {
                        VkImage image = nativeResource->getAs<VkImage>();
                        VkImageMemoryBarrier memoryBarrier = { };
                        memoryBarrier.oldLayout = getImageLayout(transition.oldState);
                        memoryBarrier.newLayout = getImageLayout(transition.newState);
                        memoryBarrier.image = image;
                        memoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                        memoryBarrier.srcAccessMask = getDesiredResourceStateAccessMask(transition.oldState);
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
                        VkBuffer buffer = nativeResource->getAs<VkBuffer>();
                        VkBufferMemoryBarrier memoryBarrier = { };
                        memoryBarrier.sType                 = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                        memoryBarrier.srcAccessMask         = getDesiredResourceStateAccessMask(transition.oldState);
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
                ClearRenderTargetHeader* clearHeader = (ClearRenderTargetHeader*)(address + sizeof(CommandHeader));

                VkClearAttachment clearAttachment = { };
                clearAttachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                clearAttachment.colorAttachment = clearHeader->renderTargetIndex;
                clearAttachment.clearValue.color.float32[0] = clearHeader->clearColor[0];
                clearAttachment.clearValue.color.float32[1] = clearHeader->clearColor[1];
                clearAttachment.clearValue.color.float32[2] = clearHeader->clearColor[2];   
                clearAttachment.clearValue.color.float32[3] = clearHeader->clearColor[3];   

                VkClearRect clearRect = { };
                clearRect.baseArrayLayer = 0;
                clearRect.layerCount = 1;
                clearRect.rect.offset.x = clearHeader->rect.x;
                clearRect.rect.offset.y = clearHeader->rect.y;
                clearRect.rect.extent.width = clearHeader->rect.width;
                clearRect.rect.extent.height = clearHeader->rect.height;

                vkCmdClearAttachments(tracker.commandbuffer, 1, &clearAttachment, 1, &clearRect);
                break;
            }
            case CommandOpcode_BindRenderTargets:
            {
                BindRenderTargetsHeader* renderTargetsHeader = (BindRenderTargetsHeader*)(address + sizeof(CommandHeader));

                if (renderTargetsHeader->numRenderTargets == 0 && renderTargetsHeader->hasDepthStencil == 0)
                {
                    continue;
                }

                // end any existing render pass.
                endRenderPass(tracker);
                flushBarriers(tracker);
                beginRenderPass(tracker, address);
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

void VulkanCommandListEncoder::endRenderPass(StateTracker& tracker)
{
    if (isInRenderPass())
    {
        vkCmdEndRendering(tracker.commandbuffer);
        setInRenderPass(false);
    }
}

void VulkanCommandListEncoder::beginRenderPass(StateTracker& tracker, UPtr renderpassAddress)
{
    BindRenderTargetsHeader* renderTargetsHeader = (BindRenderTargetsHeader*)(renderpassAddress + sizeof(CommandHeader));
    if (renderTargetsHeader->numRenderTargets == 0 && renderTargetsHeader->hasDepthStencil == 0)
    {
        return;
    }
    VkRenderingInfo renderingInfo = { };
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.colorAttachmentCount = renderTargetsHeader->numRenderTargets;
    renderingInfo.layerCount = 1;
    renderingInfo.renderArea.offset.x = 0;
    renderingInfo.renderArea.offset.y = 0;
    renderingInfo.flags = 0;

    if (renderTargetsHeader->flags & BindRenderTargetsHeader::Flag_InheritsBundles)
        renderingInfo.flags |= VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT;
    if (renderTargetsHeader->flags & BindRenderTargetsHeader::Flag_ContentsInline)
        renderingInfo.flags |= VK_RENDERING_CONTENTS_INLINE_BIT_KHR;

    VkRenderingAttachmentInfo colorAttachments[8] = { };
    VkRenderingAttachmentInfo depthAttachment = { };
    VkRenderingAttachmentInfo stencilAttachment = { };
    
    RenderPassAttachment* viewId = reinterpret_cast<RenderPassAttachment*>((UPtr)renderTargetsHeader + sizeof(BindRenderTargetsHeader));
    VkExtent2D extent = { };
    for (uint i = 0; i < renderTargetsHeader->numRenderTargets; ++i)
    {
        Descriptor* nativeView = reinterpret_cast<Descriptor*>(viewId[i].view);   
        colorAttachments[i].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachments[i].imageView = nativeView->getAs<VkImageView>();
        colorAttachments[i].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachments[i].loadOp = Vulkan::getAttachmentLoadOp(viewId[i].loadOp);
        colorAttachments[i].storeOp = Vulkan::getAttachmentStoreOp(viewId[i].storeOp);

        extent.width = Math::maximum(extent.width, nativeView->resource->getWidth());
        extent.height = Math::maximum(extent.height, nativeView->resource->getHeight());
    }

    if (renderTargetsHeader->hasDepthStencil)
    {
        Descriptor* nativeView = reinterpret_cast<Descriptor*>(&viewId[renderTargetsHeader->numRenderTargets].view);
        depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthAttachment.imageView = nativeView->getAs<VkImageView>();
        depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthAttachment.loadOp = Vulkan::getAttachmentLoadOp(viewId[renderTargetsHeader->numRenderTargets].loadOp);
        depthAttachment.storeOp = Vulkan::getAttachmentStoreOp(viewId[renderTargetsHeader->numRenderTargets].storeOp);
        stencilAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        stencilAttachment.imageView = nativeView->getAs<VkImageView>();
        stencilAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        stencilAttachment.loadOp = Vulkan::getAttachmentLoadOp(viewId[renderTargetsHeader->numRenderTargets].loadOp);
        stencilAttachment.storeOp = Vulkan::getAttachmentStoreOp(viewId[renderTargetsHeader->numRenderTargets].storeOp);
        renderingInfo.pDepthAttachment = &depthAttachment;
        renderingInfo.pStencilAttachment = &stencilAttachment;

        extent.width = Math::maximum(extent.width, nativeView->resource->getWidth());
        extent.height = Math::maximum(extent.height, nativeView->resource->getHeight());
    }

    renderingInfo.renderArea.extent = extent;
    renderingInfo.pColorAttachments = colorAttachments;

    vkCmdBeginRendering(tracker.commandbuffer, &renderingInfo);
    setInRenderPass(true);
}

void VulkanCommandListEncoder::flushBarriers(StateTracker& tracker)
{
    endRenderPass(tracker);

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