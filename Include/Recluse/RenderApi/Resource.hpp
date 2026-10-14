#ifndef RECLUSE_RENDER_API_RENDER_RESOURCE_HPP
#define RECLUSE_RENDER_API_RENDER_RESOURCE_HPP

#pragma once

#include <Recluse/Types.hpp>

#include <Recluse/RenderApi/Common.hpp>

#include <RecluseRenderApi_exports.hpp>

namespace Recluse {
namespace RenderApi {

class Resource
{
public:
    struct Description
    {
        ResourceType type;
        ResourceFormat format;
        ResourceUsage usage;
        ResourceMemoryUsage memoryUsage;
        uint width;
        uint height;
        uint depthOrArraySize;
    };

    struct MapRange
    {
        uint offset;
        uint sizeBytes;
    };

    Resource(ResourceId id) : m_id(id) { }
    virtual ~Resource() { }

    virtual void*                   map(const MapRange& range) = 0;
    virtual ResultCode              unmap(const void* ptr, const MapRange& range) = 0;
    
    virtual ResourceView            asView(const ResourceViewDescription& desc) = 0;
    virtual ResourceView            defaultView() = 0;

    ResourceId                      getId() const { return m_id; }

    virtual ResourceState           getInitialResourceState() const = 0;
    
    RecluseRenderApi_PUBLIC_API ResourceState getResourceState() const;

    virtual Resource::Description   getDescription() const = 0;

    ResourceView                  operator()(const ResourceViewDescription& desc)
    {
        return asView(desc);
    }

    ResourceView operator()()
    {
        return defaultView();
    }

private:
    ResourceId                      m_id;
};
} // RenderApi
} // Recluse
#endif // RECLUSE_RENDER_API_RENDER_RESOURCE_HPP