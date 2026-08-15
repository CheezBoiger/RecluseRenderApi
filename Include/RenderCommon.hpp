// Recluse Render Api.
#ifndef RECLUSE_RENDER_API_COMMON_HPP
#define RECLUSE_RENDER_API_COMMON_HPP

#pragma once

#include <Recluse/Types.hpp>

namespace Recluse {
namespace RenderApi {

typedef U32 PipelineId;
typedef U32 ResourceId;
typedef U32 ResourceViewId;
typedef U32 SamplerId;

enum Api
{
    Api_Unknown,
    Api_Direct3D11,
    Api_Direct3D12,
    Api_Vulkan,
    Api_OpenGL,
    Api_SoftwareRaster,
    Api_SoftwareRaytrace,
};


struct InstanceDescription
{
};
} // RenderApi
} // Recluse
#endif // RECLUSE_RENDER_API_COMMON_HPP