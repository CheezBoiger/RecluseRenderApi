#ifndef RECLUSE_VULKAN_RESOURCE_HPP
#define RECLUSE_VULKAN_RESOURCE_HPP

#pragma once

#include "VulkanCommon.hpp"

#include <Recluse/RenderApi/Resource.hpp>

#include <RecluseRenderApi_exports.hpp>
#include <unordered_map>

namespace Recluse {
namespace RenderApi {
namespace Vulkan {

class ResourceView
{
public: 
};

class VulkanResource : public Resource
{
public:
    typedef void* ResourceHandle;

    enum Type : u8 { None, Image, Buffer };

    VulkanResource() 
        : Resource(0)
        , m_type(None)
        , m_handle(nullptr) { }

    VulkanResource(VkBuffer buffer) 
        : Resource(0)
        , m_type(Buffer)
        , m_handle(reinterpret_cast<ResourceHandle>(buffer)) { }

    VulkanResource(VkImage image)
        : Resource(0)
        , m_type(Image)
        , m_handle(reinterpret_cast<ResourceHandle>(image)) { }
    
    virtual                                 ~VulkanResource();

    ResourceViewId                          asView(const ResourceViewDescription& description) override;
    ResourceViewId                          defaultView() override;

    void*                                   map(const MapRange& range) override;
    ResultCode                              unmap(const void* ptr, const MapRange& range) override;

    ResourceState                           getCurrentState() const override;
    Resource::Description                   getDescription() const override;

    Bool                                    isBuffer() const { return (m_type == Buffer); }
    Bool                                    isImage() const { return (m_type == Image); }

    static VkMemoryRequirements             queryBufferMemoryRequirements(VkDevice device, VkBuffer buffer);
    static VkMemoryRequirements             queryImageMemoryRequirements(VkDevice device, VkImage image);
    
    static VkMemoryRequirements2            queryDeviceBufferMemoryRequirements(VkDevice device, const VkBufferCreateInfo& bufferCi);
    static VkMemoryRequirements2            queryDeviceImageMemoryRequirements(VkDevice device, const VkImageCreateInfo& imageCi);
private:
    std::unordered_map<ResourceViewId, ResourceView>    m_views;
    Type                                                m_type;
    ResourceHandle                                      m_handle;
};
} // Vulkan
} // RenderApi
} // Recluse
#endif // RECLUSE_VULKAN_RESOURCE_HPP