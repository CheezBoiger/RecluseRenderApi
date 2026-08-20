//
#include <Recluse/Messaging.hpp>

#include "VulkanContext.hpp"

namespace Recluse {
namespace RenderApi {
namespace Vulkan {

VulkanContext::VulkanContext(const Description& description)
    : Context(Api::Vulkan)
    , m_instance(VK_NULL_HANDLE)
{
    initialize(description);
}

void VulkanContext::initialize(const Description& description)
{
    VkApplicationInfo applicationInfo = { };
    VkInstanceCreateInfo instanceCreateInfo = { };

    instanceCreateInfo.pApplicationInfo = &applicationInfo;

    VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance);
    if (result != VK_SUCCESS)
    {
        R_ERROR("Vulkan", "Failed to created instance, error code=%d", result);
        m_instance = nullptr;
    }
}

u32 VulkanContext::enumerateAdapterInformation(Adapter::Information* adapterInformation, u32 maxAdapters)
{
    return 0;
}

Adapter* VulkanContext::createAdapter(uint adapter)
{
    return nullptr;
}

ResultCode VulkanContext::freeAdapter(Adapter* adapter)
{
    return RecluseResult_NoImpl;
}

Bool VulkanContext::isValid() const
{
    return (m_instance != VK_NULL_HANDLE);
}
} // Vulkan
} // RenderApi
} // Recluse