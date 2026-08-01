// Recluse Render Api.
#ifndef RECLUSE_RENDER_API_INSTANCE_HPP
#define RECLUSE_RENDER_API_INSTANCE_HPP

#pragma once

#include <Recluse/Types.hpp>

namespace Recluse {
namespace RenderApi {

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


enum CommandType
{
    CommandType_NoOp,
    CommandType_Begin,
    CommandType_End,
};


struct Command
{
    CommandType type;
};


struct DrawIndexedInstancedCommand
{
    CommandType type;
    U32 indexCount;
    U32 startIndex;
    U32 instanceCount;
};
} // RenderApi
} // Recluse
#endif // RECLUSE_RENDER_API_INSTANCE_HPP