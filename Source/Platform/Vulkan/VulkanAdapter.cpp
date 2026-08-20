//
#include "VulkanAdapter.hpp"

namespace Recluse {
namespace RenderApi {
namespace Vulkan {

static uint kAdapterCounter = 0;

VulkanAdapter::VulkanAdapter(VkPhysicalDevice physDevice)
    : Adapter(++kAdapterCounter)
    , m_physicalDevice(physDevice)
{
    
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
} // Vulkan
} // RenderApi
} // Recluse