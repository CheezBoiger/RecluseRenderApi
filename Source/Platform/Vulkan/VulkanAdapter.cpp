//
#include "VulkanAdapter.hpp"

#include <Recluse/Messaging.hpp>

namespace Recluse {
namespace RenderApi {
namespace Vulkan {

static uint kAdapterCounter = 0;

VulkanAdapter::VulkanAdapter(VkPhysicalDevice physDevice)
    : Adapter(++kAdapterCounter)
    , m_physicalDevice(physDevice)
{
    initialize();
}

ResultCode VulkanAdapter::queryInformation(Information* info)
{
    return RecluseResult_Ok;
}

Bool VulkanAdapter::supportsFeature(FeatureFlag feature)
{
    return false;
}

Device* VulkanAdapter::createDevice(const DeviceDescription& description)
{
    return nullptr;
}

ResultCode VulkanAdapter::freeDevice(Device* device)
{
    return RecluseResult_Ok;
}

VkPhysicalDeviceMemoryProperties VulkanAdapter::getMemoryProperties() const
{
    return m_memoryProperties.memoryProperties;
}

VkPhysicalDeviceMemoryBudgetPropertiesEXT VulkanAdapter::getBudgetProperties() const
{
    return m_memoryBudgets;
}

void VulkanAdapter::initialize()
{
    R_ASSERT(m_physicalDevice != VK_NULL_HANDLE);

    m_memoryBudgets = { };
    m_memoryBudgets.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT;

    m_memoryProperties = { };
    m_memoryProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
    m_memoryProperties.pNext = &m_memoryBudgets;

    vkGetPhysicalDeviceMemoryProperties2(m_physicalDevice, &m_memoryProperties);
    
}
} // Vulkan
} // RenderApi
} // Recluse