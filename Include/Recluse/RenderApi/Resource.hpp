#ifndef RECLUSE_RENDER_API_RENDER_RESOURCE_HPP
#define RECLUSE_RENDER_API_RENDER_RESOURCE_HPP

#pragma once

#include <Recluse/Types.hpp>

#include <Recluse/RenderApi/Common.hpp>

namespace Recluse {
namespace RenderApi {

class Resource
{
public:

    struct MapRange
    {
        uint offset;
        uint sizeBytes;
    };

    Resource(ResourceId id) : m_id(id) { }
    virtual ~Resource() { }

    virtual ResultCode map(void** ptr, const MapRange& range) = 0;
    virtual ResultCode unmap(const void* ptr, const MapRange& range) = 0;
    
    virtual ResourceViewId asView(const ResourceViewDescription& desc) = 0;
    virtual ResourceViewId defaultView() = 0;

    ResourceId getId() const { return m_id; }

    virtual ResourceState getCurrentState() = 0;

    virtual ResourceDescription getDescription() = 0;

    ResourceViewId operator()(const ResourceViewDescription& desc)
    {
        return asView(desc);
    }

    ResourceViewId operator()()
    {
        return defaultView();
    }

private:
    ResourceId m_id;
};
} // RenderApi
} // Recluse
#endif // RECLUSE_RENDER_API_RENDER_RESOURCE_HPP