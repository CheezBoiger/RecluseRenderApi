#ifndef RECLUSE_VULKAN_RESOURCE_HPP
#define RECLUSE_VULKAN_RESOURCE_HPP

#pragma once

#include "VulkanCommon.hpp"
#include "VulkanAllocator.hpp"

#include <Recluse/RenderApi/Resource.hpp>
#include <Recluse/Threading/Threading.hpp>

#include <RecluseRenderApi_exports.hpp>
#include <unordered_map>
#include <map>

namespace Recluse {
namespace RenderApi {
namespace Vulkan {

typedef uint ViewId;
typedef U64 VulkanViewType;

struct Descriptor;

// Vulkan resource object. This object manages the structure of a vulkan buffer or image.
// It does not handle the destruction or creation of the buffer or image itself, nor does it track the state, 
// // but it does provide the necessary info required to handle the object at runtime or for prior creation in the device.
class VulkanResource : public Resource
{
public:
    typedef void* ResourceHandle;

    static const uint kNumFirstReservedIdentifers = 16;

    static uint makeResourceId();

    enum Type : u8 { None, Image, Buffer };

    VulkanResource(VkDevice device = VK_NULL_HANDLE, ResourceId id = { }) 
        : Resource(id)
        , m_device(device)
        , m_type(None)
        , m_initialResourceState(ResourceState_Unknown)
        , m_memoryAllocation({})
        , m_description({})
        , m_handle(nullptr)     { }

    VulkanResource(VkDevice device, VkBuffer buffer, ResourceId id = { }) 
        : Resource(id)
        , m_device(device)
        , m_type(Buffer)
        , m_initialResourceState(ResourceState_Unknown)
        , m_memoryAllocation({})
        , m_description({})
        , m_handle(reinterpret_cast<ResourceHandle>(buffer)) { }

    VulkanResource(VkDevice device, VkImage image, ResourceId id = { })
        : Resource(id) 
        , m_device(device)
        , m_type(Image)
        , m_initialResourceState(ResourceState_Unknown)
        , m_memoryAllocation({})
        , m_description({})
        , m_handle(reinterpret_cast<ResourceHandle>(image)) { }
    
    virtual                                 ~VulkanResource();

    ResourceView                           asView(const ResourceViewDescription& description) override;
    ResourceView                           defaultView() override;
    VkFormat                               getDefaultViewFormat() const { return Vulkan::getVulkanFormat(getDescription().format); }

    void*                                   map(const MapRange& range) override;
    ResultCode                              unmap(const void* ptr, const MapRange& range) override;

    ResourceState                           getInitialResourceState() const override;
    const Resource::Description&            getDescription() const override;
    void                                    setDescription(const Resource::Description& description) { m_description = description; }

    Bool                                    isBuffer() const { return (m_type == Buffer); }
    Bool                                    isImage() const { return (m_type == Image); }
    Bool                                    isValid() const { return (m_handle != nullptr); }
    const Allocation&                       getMemory() const { return m_memoryAllocation; }
    U32                                     getWidth() const { return m_description.width; }
    U32                                     getHeight() const { return m_description.height; }
    ResourceFormat                          getDefaultFormat() const { return m_description.format; }
    
    template<typename VulkanResourceType>
    VulkanResourceType                      getAs() { return reinterpret_cast<VulkanResourceType>(m_handle); }

    ResourceHandle                          raw() const { return m_handle; }

    static VkMemoryRequirements             queryBufferMemoryRequirements(VkDevice device, VkBuffer buffer);
    static VkMemoryRequirements             queryImageMemoryRequirements(VkDevice device, VkImage image);
    
    static VkMemoryRequirements2            queryDeviceBufferMemoryRequirements(VkDevice device, const VkBufferCreateInfo& bufferCi);
    static VkMemoryRequirements2            queryDeviceImageMemoryRequirements(VkDevice device, const VkImageCreateInfo& imageCi);
    
    void                                    setInitialResourceState(ResourceState state) { m_initialResourceState = state; }
private:
    Type                                                    m_type;
    ResourceHandle                                          m_handle;
    ResourceState                                           m_initialResourceState;
    Allocation                                              m_memoryAllocation;
    Resource::Description                                   m_description;
    VkDevice                                                m_device;
    std::map<ViewId, Descriptor>                            m_views;
    
};

typedef struct Descriptor
{
    typedef U64 ViewHandle;
    static_assert(sizeof(ViewHandle) <= sizeof(ResourceView), "ViewHandle should be same size as ResourceView");

    VulkanResource::Type resourceType;
    ResourceViewType viewType;
    ViewHandle handle;
    VulkanResource* resource;

    template<typename Type>
    Type getAs() const { return reinterpret_cast<Type>(handle); }

    static VkImageAspectFlags       resolveImageAspectFlags(VkFormat originalFormat, ResourceFormat requestedFormat);
} Descriptor;
} // Vulkan
} // RenderApi
} // Recluse
#endif // RECLUSE_VULKAN_RESOURCE_HPP