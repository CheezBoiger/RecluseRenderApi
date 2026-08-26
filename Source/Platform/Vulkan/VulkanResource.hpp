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

class VulkanResourceView
{
public: 
};

// Vulkan resource object. This object manages the state and structure of a vulkan buffer or image.
// It does not handle the destruction or creation of the buffer or image itself, but it does provide the 
// necessary info required to handle the object at runtime or for prior creation in the device.
class VulkanResource : public Resource
{
public:
    typedef void* ResourceHandle;
    static const uint kNumFirstReservedIdentifers = 16;

    static uint makeResourceId();

    enum Type : u8 { None, Image, Buffer };

    VulkanResource(ResourceId id = { }) 
        : Resource(id)
        , m_type(None)
        , m_currentState(ResourceState_Unknown)
        , m_handle(nullptr) { }

    VulkanResource(VkBuffer buffer, ResourceId id = { }) 
        : Resource(id)
        , m_type(Buffer)
        , m_currentState(ResourceState_Unknown)
        , m_handle(reinterpret_cast<ResourceHandle>(buffer)) { }

    VulkanResource(VkImage image, ResourceId id = { })
        : Resource(id)
        , m_type(Image)
        , m_currentState(ResourceState_Unknown)
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
    
    template<typename VulkanResourceType>
    VulkanResourceType                      get() { return reinterpret_cast<VulkanResourceType>(m_handle); }

    static VkMemoryRequirements             queryBufferMemoryRequirements(VkDevice device, VkBuffer buffer);
    static VkMemoryRequirements             queryImageMemoryRequirements(VkDevice device, VkImage image);
    
    static VkMemoryRequirements2            queryDeviceBufferMemoryRequirements(VkDevice device, const VkBufferCreateInfo& bufferCi);
    static VkMemoryRequirements2            queryDeviceImageMemoryRequirements(VkDevice device, const VkImageCreateInfo& imageCi);
private:
    std::unordered_map<ResourceViewId, VulkanResourceView>  m_views;
    Type                                                    m_type;
    ResourceHandle                                          m_handle;
    ResourceState                                           m_currentState;
};
} // Vulkan
} // RenderApi
} // Recluse
#endif // RECLUSE_VULKAN_RESOURCE_HPP