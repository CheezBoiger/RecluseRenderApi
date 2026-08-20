#ifndef RECLUSE_VULKAN_INSTANCE_HPP
#define RECLUSE_VULKAN_INSTANCE_HPP

#pragma once

#include "VulkanCommon.hpp"
#include "VulkanContext.hpp"

#include <RecluseRenderApi_exports.hpp>

extern "C" {
RecluseRenderApi_PUBLIC_API Recluse::RenderApi::Context*    createContext(const Recluse::RenderApi::Context::Description& description);
RecluseRenderApi_PUBLIC_API Recluse::ResultCode             freeContext(Recluse::RenderApi::Context* context);
}
#endif // RECLUSE_VULKAN_INSTANCE_HPP