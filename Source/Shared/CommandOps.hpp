#ifndef RECLUSE_RENDER_API_COMMAND_OPS_HPP
#define RECLUSE_RENDER_API_COMMAND_OPS_HPP

#pragma once

#include <Recluse/Types.hpp>

#include <type_traits>
#include <limits>

namespace Recluse {
namespace RenderApi {

// Command opcodes are the opcodes that are used to identify the command that is being executed.
// They should be used to help identify the necessary implementation for the Api.
enum CommandOpcode : u16
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

struct CommandHeader
{
    CommandOpcode opcode;
    U16           size;

    // Gets the data size of the command, this is the header + command data.
    // Ideally should not be so large.
    template<typename Type>
    static U32 dataSize()
    {
        static_assert(
            !std::is_same<Type, CommandHeader>::value, 
            "Type must not be the same as CommandHeader!");
        
        static_assert(
            (sizeof(Type) + sizeof(CommandHeader)) < std::numeric_limits<uint16_t>::max(), 
            "Command data size should not exceed 64 KB!");

        return sizeof(Type) + sizeof(CommandHeader);
    }

    template<typename Type>
    static Type* dataOffset(void* header)
    {
        return reinterpret_cast<Type*>(reinterpret_cast<UPtr>(header) + sizeof(CommandHeader));
    }
};

struct BindPipelineCommand
{
    PipelineId      pipeline;
};

struct DrawIndexedInstancedCommand
{
    U32             indexCount;
    U32             startIndex;
    U32             instanceCount;
    U32             startInstance;
    uint            baseVertex;
};

struct DrawInstancedCommand
{
    uint vertexCount;
    uint instanceCount;
    uint baseVertex;
    uint baseInstance;
};

struct DispatchCommand
{
    U32             x;
    U32             y;
    U32             z;
};

struct BindResourceTableCommand
{
    UPtr            resourceTablePtr;
    uint            sizeBytes;
};

struct BindSamplerTableCommand
{
    UPtr            samplerTablePtr;
    uint            sizeBytes;
};

struct BindRenderTargetsHeader
{
    u8 numRenderTargets : 7;
    u8 hasDepthStencil : 1;
};
} // RenderApi
} // Recluse
#endif // RECLUSE_RENDER_API_COMMAND_OPS_HPP