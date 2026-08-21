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
    VulkanAdapter(VkPhysicalDevice physicalDevice = VK_NULL_HANDLE);

    virtual ResultCode  queryInformation(Information* info) override;
    virtual Bool        supportsFeature(FeatureFlag feature) override;
    virtual Device*     createDevice(const DeviceDescription& desc) override;

    virtual ResultCode  freeDevice(Device* device) override;
    virtual Bool        isValid() const { return m_physicalDevice != VK_NULL_HANDLE; }

    VkPhysicalDevice    operator()() const { return m_physicalDevice; }
    VkPhysicalDevice    get() const { return m_physicalDevice; }

    VkPhysicalDeviceMemoryProperties getMemoryProperties() const;
    VkPhysicalDeviceMemoryBudgetPropertiesEXT getBudgetProperties() const;

private:

    void initialize();

    VkPhysicalDevice                            m_physicalDevice;
    VkPhysicalDeviceMemoryProperties2           m_memoryProperties; // Static total memory for each memory heap. Doesn't take realtime into account.
    VkPhysicalDeviceMemoryBudgetPropertiesEXT   m_memoryBudgets;    // this is realtime memory budgets that are taken into account.
};
} // Vulkan
} // RenderApi
} // Recluse
#endif // RECLUSE_VULKAN_ADAPTER_HPP