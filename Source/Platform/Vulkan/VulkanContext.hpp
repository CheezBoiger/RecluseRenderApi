#ifndef RECLUSE_VULKAN_CONTEXT_HPP
#define RECLUSE_VULKAN_CONTEXT_HPP
#pragma once

#include <Recluse/RenderApi/Context.hpp>
#include "VulkanCommon.hpp"

#include <RecluseRenderApi_exports.hpp>

namespace Recluse {
namespace RenderApi {
namespace Vulkan {


class RecluseRenderApi_PUBLIC_API VulkanContext : public Context
{
public:
    VulkanContext(const Description& description);

    void initialize(const Description& description);
    void destroy();

    u32 enumerateAdapterInformation(Adapter::Information* adapterInformation, u32 maxAdapters) override;

    Adapter* createAdapter(uint adapter) override;
    ResultCode freeAdapter(Adapter* adapter) override;

    Bool isValid() const override;

private:
    // Instance for vulkan.
    VkInstance m_instance;
};
} // Vulkan
} // RenderApi
} // Recluse
#endif // RECLUSE_VULKAN_CONTEXT_HPP