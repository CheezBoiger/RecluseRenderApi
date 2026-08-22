#ifndef RECLUSE_VULKAN_ADAPTER_HPP
#define RECLUSE_VULKAN_ADAPTER_HPP

#pragma once

#include <Recluse/RenderApi/Common.hpp>
#include <Recluse/RenderApi/Adapter.hpp>

#include "VulkanCommon.hpp"

#include <RecluseRenderApi_exports.hpp>
#include <vector>
#include <list>
#include <map>
#include <typeinfo>

namespace Recluse {
namespace RenderApi { 
namespace Vulkan {

class VulkanContext;

// Adapter hardware.
class RecluseRenderApi_PUBLIC_API VulkanAdapter : public Adapter
{
public:
    static VkPhysicalDeviceProperties           gatherProperties(VkPhysicalDevice physicalDevice);
    static Information                          gatherInformation(const VkPhysicalDeviceProperties& properties, uint index);
    static std::vector<VkExtensionProperties>   gatherExtensionProperties(VkPhysicalDevice physicalDevice);
    Bool                                        checkSupportsDeviceExtension(VkPhysicalDevice physicalDevice, const char* ext);



    VulkanAdapter(VulkanContext* context = nullptr, VkPhysicalDevice physicalDevice = VK_NULL_HANDLE, uint adapterIndex = -1);

    virtual ResultCode  queryInformation(Information* info) override;
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

// Features creation.
class Features
{
public:
    Features()
    {
        physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        physicalDeviceFeatures2.pNext = nullptr;
    }

    ~Features()
    {
        for (auto it = features.begin(); it != features.end(); ++it)
        {
            delete *(it);
        }
    }

    template<typename FeatureType>
    FeatureType* add()
    {
        std::string name = typeid(FeatureType).name();
        FeatureType* type = nullptr;
        auto it = featureMap.find(name);
        if (it == featureMap.end())
        {
            features.push_back(new FeatureType{});
            featureMap.insert(std::make_pair(name, features.back()));
            type = (FeatureType*)features.back();
            type->pNext = physicalDeviceFeatures2.pNext;
            physicalDeviceFeatures2.pNext = type;
        }
        else 
        {
            type = (FeatureType*)it->second;
        }
        return type;
    }

    template<typename FeatureType>
    FeatureType* find()
    {
        std::string name = typeid(FeatureType).name();
        auto it = featureMap.find(name);
        if (it != featureMap.end())
            return (FeatureType*)it->second;
        return nullptr;
    }

    VkPhysicalDeviceFeatures2& getDeviceFeatures()
    {
        return physicalDeviceFeatures2;
    }

    VkPhysicalDeviceFeatures2& operator()() { return getDeviceFeatures(); }
private:
    VkPhysicalDeviceFeatures2 physicalDeviceFeatures2;
    
    std::map<std::string, void*> featureMap;
    std::list<void*> features;
};
} // Vulkan
} // RenderApi
} // Recluse
#endif // RECLUSE_VULKAN_ADAPTER_HPP