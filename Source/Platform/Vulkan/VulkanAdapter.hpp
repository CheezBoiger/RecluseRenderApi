#ifndef RECLUSE_VULKAN_ADAPTER_HPP
#define RECLUSE_VULKAN_ADAPTER_HPP

#pragma once

#include <Recluse/RenderApi/Common.hpp>
#include <Recluse/RenderApi/Adapter.hpp>

#include "VulkanCommon.hpp"

#include <RecluseRenderApi_exports.hpp>

namespace Recluse {
namespace RenderApi { 
namespace Vulkan {

class RecluseRenderApi_PUBLIC_API VulkanAdapter : public Adapter
{
public:
    VulkanAdapter(VkPhysicalDevice physicalDevice = nullptr);

    virtual ResultCode  queryInformation(Information* info) override;
    virtual Bool        supportsFeature(FeatureFlag feature) override;
    virtual Device*     createDevice(const DeviceDescription& desc) override;

    virtual ResultCode  freeDevice(Device* device) override;
    virtual Bool        isValid() const { return m_physicalDevice != VK_NULL_HANDLE; }

    VkPhysicalDevice    operator()() const { return m_physicalDevice; }
    VkPhysicalDevice    get() const { return m_physicalDevice; }
private:
    VkPhysicalDevice    m_physicalDevice;
};
} // Vulkan
} // RenderApi
} // Recluse
#endif // RECLUSE_VULKAN_ADAPTER_HPP