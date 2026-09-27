// 
#include "VulkanResource.hpp"
#include <Recluse/Threading/Threading.hpp>

namespace Recluse {
namespace RenderApi {
namespace Vulkan {

static MutexGuard resourceCounterMutex = { };
static uint kResourceCounter = 0;

VkMemoryRequirements VulkanResource::queryBufferMemoryRequirements(VkDevice device, VkBuffer buffer)
{
    //
    VkMemoryRequirements requirements = { };
    vkGetBufferMemoryRequirements(device, buffer, &requirements);
    return requirements;
}

VkMemoryRequirements VulkanResource::queryImageMemoryRequirements(VkDevice device, VkImage image)
{
    //
    VkMemoryRequirements requirements = { };
    vkGetImageMemoryRequirements(device, image, &requirements);
    return requirements;
}

VkMemoryRequirements2 VulkanResource::queryDeviceBufferMemoryRequirements(VkDevice device, const VkBufferCreateInfo& createCi)
{
    VkMemoryRequirements2 memoryRequirements = { };
    VkDeviceBufferMemoryRequirements bufferRequirements = { };
    
    bufferRequirements.sType = VK_STRUCTURE_TYPE_DEVICE_BUFFER_MEMORY_REQUIREMENTS;
    bufferRequirements.pCreateInfo = &createCi;

    vkGetDeviceBufferMemoryRequirements(device, &bufferRequirements, &memoryRequirements);

    return memoryRequirements;
}

VkMemoryRequirements2 VulkanResource::queryDeviceImageMemoryRequirements(VkDevice device, const VkImageCreateInfo& createCi)
{
    VkMemoryRequirements2 memoryRequirements = { };
    VkDeviceImageMemoryRequirements imageRequirements = { };
    
    imageRequirements.sType = VK_STRUCTURE_TYPE_DEVICE_IMAGE_MEMORY_REQUIREMENTS;
    imageRequirements.pCreateInfo = &createCi;
    imageRequirements.planeAspect = VK_IMAGE_ASPECT_NONE; // This value is ignored unless pCreateInfo->tiling is VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT
    
    vkGetDeviceImageMemoryRequirements(device, &imageRequirements, &memoryRequirements);

    return memoryRequirements;
}

VulkanResource::~VulkanResource()
{
}

void* VulkanResource::map(const MapRange& range)
{
    return nullptr;
}

ResultCode VulkanResource::unmap(const void* ptr, const MapRange& range)
{
    return RecluseResult_NoImpl;
}

ResourceViewId VulkanResource::asView(const ResourceViewDescription& description)
{
    return 0;
}

ResourceViewId VulkanResource::defaultView()
{
    return 0;
}

ResourceState VulkanResource::getInitialResourceState() const
{
    return m_initialResourceState;
}

Resource::Description VulkanResource::getDescription() const
{
    return { };
}

uint VulkanResource::makeResourceId()
{
    ScopedLock _(resourceCounterMutex);
    return kNumFirstReservedIdentifers + kResourceCounter++;
}

ResourceState ResourceStateDatabase::queryCurrentResourceState(ResourceId id)
{
    ScopedLock _(m_resourceMutex);
    auto it = m_currentResourceStates.find(id.value);
    if (it != m_currentResourceStates.end())
    {
        return it->second;
    }
    return ResourceState_Unknown;
}

void ResourceStateDatabase::setCurrentResourceState(ResourceId id, ResourceState state)
{
    ScopedLock _(m_resourceMutex);
    // The current state is newer than the one we are trying to set, so we don't update it.
    m_currentResourceStates[id.value] = state;
}

Bool ResourceStateDatabase::hasResourceState(ResourceId id)
{
    ScopedLock _(m_resourceMutex);
    auto it = m_currentResourceStates.find(id.value);
    return (it != m_currentResourceStates.end());
}
} // Vulkan
} // RenderApi 
} // Recluse