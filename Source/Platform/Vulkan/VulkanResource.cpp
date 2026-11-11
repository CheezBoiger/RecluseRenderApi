// 
#include "VulkanResource.hpp"
#include <Recluse/Threading/Threading.hpp>
#include <Recluse/Serialization/Hasher.hpp>

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
    for (const auto& it : m_views)
    {
        if (it.second.resourceType == VulkanResource::Image)
        {
            VkImageView view = it.second.getAs<VkImageView>();
            vkDestroyImageView(m_device, view, nullptr);
        }
    }
    m_views.clear();
}

void* VulkanResource::map(const MapRange& range)
{
    return nullptr;
}

ResultCode VulkanResource::unmap(const void* ptr, const MapRange& range)
{
    return RecluseResult_NoImpl;
}

ResourceView VulkanResource::asView(const ResourceViewDescription& description)
{
    ViewId id = recluseHash32(&description, sizeof(const ResourceViewDescription));
    auto it = m_views.find(id);

    if (it == m_views.end())
    {
        if (isImage())
        {
            VkImageViewCreateInfo viewCi = { };
            viewCi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewCi.format = Vulkan::getVulkanFormat(description.format);
            viewCi.viewType = Vulkan::getViewTypeFromDimension(description.dimension);
            viewCi.image = getAs<VkImage>();
            viewCi.flags = 0;
            viewCi.pNext = nullptr;
            viewCi.components.r = VK_COMPONENT_SWIZZLE_R;
            viewCi.components.g = VK_COMPONENT_SWIZZLE_G;
            viewCi.components.b = VK_COMPONENT_SWIZZLE_B;
            viewCi.components.a = VK_COMPONENT_SWIZZLE_A;
            viewCi.subresourceRange.aspectMask = Descriptor::resolveImageAspectFlags(getDefaultViewFormat(), description.format);
            viewCi.subresourceRange.levelCount = description.mipLevelCount;
            viewCi.subresourceRange.baseMipLevel = description.baseMipLevel;
            viewCi.subresourceRange.baseArrayLayer = description.baseArrayLayer;
            viewCi.subresourceRange.layerCount = description.arrayLayerCount;
            VkImageView view = VK_NULL_HANDLE;
            vkCreateImageView(m_device, &viewCi, nullptr, &view);
            
            Descriptor viewId = { };
            viewId.resourceType = VulkanResource::Image;
            viewId.viewType = description.type;
            viewId.handle = reinterpret_cast<Descriptor::ViewHandle>(view);
            viewId.format = description.format;
            viewId.resource = this;

            m_views.insert(std::make_pair(id, viewId));
            it = m_views.find(id);
        }
    }
    
    return reinterpret_cast<ResourceView>(&it->second);
}

ResourceView VulkanResource::defaultView()
{
    return 0;
}

ResourceState VulkanResource::getInitialResourceState() const
{
    return m_initialResourceState;
}

const Resource::Description& VulkanResource::getDescription() const
{
    return m_description;
}

uint VulkanResource::makeResourceId()
{
    ScopedLock _(resourceCounterMutex);
    return kNumFirstReservedIdentifers + kResourceCounter++;
}

VkImageAspectFlags Descriptor::resolveImageAspectFlags(VkFormat originalFormat, ResourceFormat requestedFormat)
{
    VkImageAspectFlags aspect = Vulkan::getImageAspectFlags(originalFormat);
    switch (requestedFormat)
    {
        case Recluse::RenderApi::ResourceFormat_R24_Unorm_X8_Typeless:
            aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
            break;
        case Recluse::RenderApi::ResourceFormat_X24_Typeless_S8_Uint:
            aspect = VK_IMAGE_ASPECT_STENCIL_BIT;
            break;
    }
    return aspect;
}
} // Vulkan
} // RenderApi 
} // Recluse