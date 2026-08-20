#ifndef RECLUSE_VULKAN_COMMON_HPP
#define RECLUSE_VULKAN_COMMON_HPP
#pragma once

#include <Recluse/Arch.hpp>
#include <Recluse/Types.hpp>

#if defined(RECLUSE_WINDOWS)
    #define VK_USE_PLATFORM_WIN32_KHR 1
#elif defined(RECLUSE_LINUX)
    #error "No linux impl yet!"
#endif

#include <vulkan/vulkan.h>

namespace Recluse {
namespace RenderApi {
namespace Vulkan {

} // Vulkan
} // RenderApi
} // Recluse
#endif // RECLUSE_VULKAN_COMMON_HPP