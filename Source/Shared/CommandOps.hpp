#ifndef RECLUSE_RENDER_API_COMMAND_OPS_HPP
#define RECLUSE_RENDER_API_COMMAND_OPS_HPP

#pragma once

#include <Recluse/Types.hpp>

#include <type_traits>
#include <limits>

namespace Recluse {
namespace RenderApi {

// Command opcodes are the operations that are used to identify the command that is being executed.
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

    CommandOpcode_ClearRenderTarget,
    CommandOpcode_ClearDepthStencil,

    CommandOpcode_Dispatch,
    CommandOpcode_CopyResource,
    CommandOpcode_CopySubresource,
    CommandOpcode_BeginRenderPass,
    CommandOpcode_EndRenderPass,
    CommandOpcode_ExecuteBundles,

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
    // Opcode is the operation instruction of this command.
    CommandOpcode opcode;

    // Size defines the actual size of the data packet, excluding this header.
    // Be sure to offset with both sizeof(CommandHeader) + header->size, as you should 
    // include the entire packet, or use packetSizeBytes().
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

    static void* offsetOf(void* header)
    {
        return reinterpret_cast<void*>(reinterpret_cast<UPtr>(header) + sizeof(CommandHeader));
    }

    // Returns the total packet size, including the header, in bytes.
    static uint packetSizeBytes(CommandHeader* header)
    {
        return (uint)(sizeof(CommandHeader) + reinterpret_cast<CommandHeader*>(header)->size);
    }
};

struct CommandListDescription
{
    CommandType type;
    CommandInstance instance;
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
    u32 numRenderTargets : 7;
    u32 hasDepthStencil : 1;
    u32 reserved : 24;
};

struct Transition
{
    Resource*       resource;
    ResourceState   resourceState;
};

struct BarrierTransitionHeader
{
    uint numTransitions;
};

struct BundlesHeader
{
    uint numBundles;
};

struct ClearRenderTargetHeader
{
    Rect rect;
    F32  clearColor[4];
    uint renderTargetIndex;
};

struct ClearDepthStencilHeader
{
    Rect        rect;
    ClearFlags  clearFlags;
    F32         clearDepth;
    U8          clearStencil;
};
} // RenderApi
} // Recluse
#endif // RECLUSE_RENDER_API_COMMAND_OPS_HPP