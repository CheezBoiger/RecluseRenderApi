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
    
    virtual ResourceViewId asView(const ResourceViewDescription& desc) = 0;
private:
};
} // RenderApi
} // Recluse
#endif // RECLUSE_RENDER_API_RENDER_RESOURCE_HPP