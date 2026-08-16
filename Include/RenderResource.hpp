#ifndef RECLUSE_RENDER_API_RENDER_RESOURCE_HPP
#define RECLUSE_RENDER_API_RENDER_RESOURCE_HPP

#pragma once

#include <Recluse/Types.hpp>

#include "RenderCommon.hpp"

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

    virtual ResultCode map(void** ptr, const MapRange& range) = 0;
    virtual ResultCode unmap(const void* ptr, const MapRange& range) = 0;
    
    virtual ResourceViewId asView(const ResourceViewDescription& desc) = 0;
    virtual ResourceViewId defaultView() = 0;

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
};
} // RenderApi
} // Recluse
#endif // RECLUSE_RENDER_API_RENDER_RESOURCE_HPP