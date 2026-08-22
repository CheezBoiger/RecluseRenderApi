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
} // Vulkan
} // RenderApi
} // Recluse
#endif // RECLUSE_VULKAN_COMMON_HPP