#ifndef RECLUSE_RENDER_API_COMMAND_OPS_HPP
#define RECLUSE_RENDER_API_COMMAND_OPS_HPP

#pragma once

#include <Recluse/Types.hpp>

namespace Recluse {
namespace RenderApi {


// Command opcodes are the opcodes that are used to identify the command that is being executed.
// They should be used to help identify the necessary implementation for the Api.
enum CommandOpcode
{
    CommandOpcode_NoOp = 0,
    CommandOpcode_Begin,
    CommandOpcode_End,
    CommandOpcode_DrawIndexedInstanced,
    CommandOpcode_DrawInstanced,

    CommandOpcode_BindPipeline,
    CommandOpcode_BindResourceTable,
    CommandOpcode_BindSamplerTable,
    CommandOpcode_BindRenderTargets,

    CommandOpcode_Dispatch,
    CommandOpcode_CopyResource,
    CommandOpcode_CopySubresource,
    CommandOpcode_BeginRenderPass,
    CommandOpcode_EndRenderPass,

    CommandOpcode_BarrierTransition,
    CommandOpcode_BarrierAliasing,

    CommandOpcode_DispatchMesh,
    CommandOpcode_DispatchRays,

    CommandOpcode_DrawIndexedInstancedIndirect,
    CommandOpcode_DrawInstancedIndirect,
    CommandOpcode_DispatchIndirect,
    CommandOpcode_DispatchMeshIndirect,

    CommandOpcode_Count,
};


struct Command
{
    CommandOpcode     type;
};


struct BeginCommand
{
    CommandOpcode     type;
};


struct BindPipelineCommand
{
    CommandOpcode     type;
    PipelineId      pipeline;
};


struct DrawIndexedInstancedCommand
{
    CommandOpcode     type;
    U32             indexCount;
    U32             startIndex;
    U32             instanceCount;
};


struct DispatchCommand
{
    CommandOpcode     type;
    U32             x;
    U32             y;
    U32             z;
};


struct BindResourceTableCommand
{
    CommandOpcode     type;
    UPtr            resourceTablePtr;
    uint            sizeBytes;
};

struct BindSamplerTableCommand
{
    CommandOpcode     type;
    UPtr            samplerTablePtr;
    uint            sizeBytes;
};
} // RenderApi
} // Recluse
#endif // RECLUSE_RENDER_API_COMMAND_OPS_HPP