#ifndef RECLUSE_VULKAN_COMMON_HPP
#define RECLUSE_VULKAN_COMMON_HPP
#pragma once

#include <Recluse/Arch.hpp>
#include <Recluse/Types.hpp>

#include <Recluse/RenderApi/Common.hpp>

#if defined(RECLUSE_WINDOWS)
    #define VK_USE_PLATFORM_WIN32_KHR 1
#elif defined(RECLUSE_LINUX)
    #error "No linux impl yet!"
#endif

#include <vulkan/vulkan.h>

#define R_VULKAN_API_VERSION(major, minor, patch) VK_MAKE_API_VERSION(0, major, minor, patch) 
#define R_VULKAN_DEFAULT_VERSION() VK_API_VERSION_1_1

namespace Recluse {
namespace RenderApi {
namespace Vulkan {

extern VkFormat                 getVulkanFormat(ResourceFormat format);
extern ResourceFormat           getResourceFormat(VkFormat format);
extern VkImageLayout            getImageLayout(ResourceState state);

static VkAccessFlags getDesiredResourceStateAccessMask(ResourceState state)
{
    switch (state)
    {
        case ResourceState_CopyDestination:         return VK_ACCESS_TRANSFER_WRITE_BIT;
        case ResourceState_CopySource:              return VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_HOST_WRITE_BIT;
        case ResourceState_ConstantBuffer:          return VK_ACCESS_UNIFORM_READ_BIT;
        case ResourceState_IndexBuffer:             return VK_ACCESS_INDEX_READ_BIT;
        case ResourceState_DepthStencilWrite:       return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        case ResourceState_IndirectBuffer:          return VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
        case ResourceState_RenderTarget:            return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
        case ResourceState_UnorderedAccess:         return VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
        case ResourceState_VertexBuffer:            return VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
        case ResourceState_Common:                  return VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;
        case ResourceState_ShaderResource:          return VK_ACCESS_SHADER_READ_BIT;
        case ResourceState_DepthStencilReadOnly:    return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        case ResourceState_Present:                 return VK_ACCESS_MEMORY_READ_BIT;
    }

    return VK_ACCESS_MEMORY_READ_BIT;
}

static VkPipelineStageFlags getDestinationPipelineStage(VkAccessFlags access)
{
    // for special cases.
    // TODO: Make this the mandatory path, and remove the switch.
    VkPipelineStageFlags flags = VK_PIPELINE_STAGE_NONE;
    if (access & VK_ACCESS_TRANSFER_WRITE_BIT || access & VK_ACCESS_TRANSFER_READ_BIT)
        flags |= VK_PIPELINE_STAGE_TRANSFER_BIT;

    if (access & VK_ACCESS_COLOR_ATTACHMENT_READ_BIT || access & VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
        flags |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    if (access & VK_ACCESS_INDEX_READ_BIT)
        flags |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
        
    if (access & VK_ACCESS_HOST_READ_BIT || access & VK_ACCESS_HOST_WRITE_BIT)
        flags |= VK_PIPELINE_STAGE_HOST_BIT;

    if (access & VK_ACCESS_SHADER_READ_BIT || access & VK_ACCESS_SHADER_WRITE_BIT)
        flags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;

    if (access & VK_ACCESS_UNIFORM_READ_BIT)
        flags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;

    if (access & VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT || access & VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)
        flags |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

    if (access & VK_ACCESS_MEMORY_READ_BIT || access & VK_ACCESS_MEMORY_WRITE_BIT)
        flags |= VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    return flags;
}
} // Vulkan
} // RenderApi
} // Recluse
#endif // RECLUSE_VULKAN_COMMON_HPP