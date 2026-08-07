#ifndef RECLUSE_RENDER_API_COMMAND_OPS_HPP
#define RECLUSE_RENDER_API_COMMAND_OPS_HPP

#pragma once

#include <Recluse/Types.hpp>

namespace Recluse {
namespace RenderApi {


enum CommandType
{
    CommandType_NoOp,
    CommandType_Begin,
    CommandType_End,
    CommandType_DrawIndexedInstanced,
    CommandType_DrawInstanced,
    CommandType_BindPipeline,
    CommandType_BindResourceTable,
    CommandType_BindSamplerTable,
    CommandType_Dispatch,
    CommandType_CopyResource,
    CommandType_CopySubresource,
    CommandType_BeginRenderPass,
    CommandType_EndRenderPass,

    CommandType_DispatchMesh,
    CommandType_DispatchRays,

    CommandType_DrawIndexedInstancedIndirect,
    CommandType_DrawInstancedIndirect,
    CommandType_DispatchIndirect,
    CommandType_DispatchMeshIndirect,

    CommandType_Count,
};


struct Command
{
    CommandType     type;
};


struct BeginCommand
{
    CommandType     type;
};


struct DrawIndexedInstancedCommand
{
    CommandType     type;
    U32             indexCount;
    U32             startIndex;
    U32             instanceCount;
};


struct DispatchCommand
{
    CommandType     type;
    U32             x;
    U32             y;
    U32             z;
};
} // RenderApi
} // Recluse
#endif // RECLUSE_RENDER_API_COMMAND_OPS_HPP