#ifndef RECLUSE_VULKAN_ADAPTER_HPP
#define RECLUSE_VULKAN_ADAPTER_HPP

#pragma once

#include <Recluse/RenderApi/Common.hpp>
#include <Recluse/RenderApi/Adapter.hpp>

#include "VulkanCommon.hpp"

#include <RecluseRenderApi_exports.hpp>
#include <vector>

namespace Recluse {
namespace RenderApi { 
namespace Vulkan {

class VulkanContext;

class RecluseRenderApi_PUBLIC_API VulkanAdapter : public Adapter
{
public:
    static VkPhysicalDeviceProperties           gatherProperties(VkPhysicalDevice physicalDevice);
    static Information                          gatherInformation(const VkPhysicalDeviceProperties& properties, uint index);
    static std::vector<VkExtensionProperties>   gatherExtensionProperties(VkPhysicalDevice physicalDevice);

    VulkanAdapter(VulkanContext* context = nullptr, VkPhysicalDevice physicalDevice = VK_NULL_HANDLE, uint adapterIndex = -1);

    virtual ResultCode  queryInformation(Information* info) override;
    virtual Bool        supportsFeature(FeatureFlag feature) override;
    virtual Device*     createDevice(const Device::Description& desc) override;

    virtual ResultCode  freeDevice(Device* device) override;
    virtual Bool        isValid() const { return m_physicalDevice != VK_NULL_HANDLE; }

    VkPhysicalDevice    operator()() const { return m_physicalDevice; }
    VkPhysicalDevice    get() const { return m_physicalDevice; }

    VkPhysicalDeviceMemoryProperties getMemoryProperties() const;
    VkPhysicalDeviceMemoryBudgetPropertiesEXT getBudgetProperties() const;

    VulkanContext*      getContext() const { return m_context; }

private:

    void initialize();

    VulkanContext*                              m_context;

    VkPhysicalDevice                            m_physicalDevice;
    VkPhysicalDeviceMemoryProperties2           m_memoryProperties; // Static total memory for each memory heap. Doesn't take realtime into account.
    VkPhysicalDeviceMemoryBudgetPropertiesEXT   m_memoryBudgets;    // this is realtime memory budgets that are taken into account.
    uint                                        m_adapterIndex;
};
} // Vulkan
} // RenderApi
} // Recluse
#endif // RECLUSE_VULKAN_ADAPTER_HPP