#ifndef RECLUSE_RESOURCE_STATE_HPP
#define RECLUSE_RESOURCE_STATE_HPP

#pragma once

#include <Recluse/RenderApi/Common.hpp>
#include <Recluse/RenderApi/Resource.hpp>
#include <Recluse/Types.hpp>

namespace Recluse {
namespace RenderApi {

namespace Database {

ResourceState   obtainResourceState(ResourceId id);
void            setResourceState(ResourceId id, ResourceState state);

} // Database
} // RenderApi
} // Recluse
#endif // RECLUSE_RESOURCE_STATE_HPP