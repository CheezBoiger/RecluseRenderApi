#ifndef RECLUSE_RENDER_API_RENDER_COMMANDLIST_HPP
#define RECLUSE_RENDER_API_RENDER_COMMANDLIST_HPP

#pragma once

#include <Recluse/RenderApi/Common.hpp>
#include <Recluse/Types.hpp>
#include <Recluse/Memory/LinearScratchMemory.hpp>

#include <RecluseRenderApi_exports.hpp>

namespace Recluse {
namespace RenderApi {

class Device;
class Pipeline;
class Resource;

// Chunk defines the base and size of a chunk of commands.
struct CommandStreamChunk
{
    UPtr baseAddress = 0;
    U32  sizeBytes = 0;
};

struct ResourceTransition
{
    Resource* resource;
    ResourceState newState;
};

// CommandList is actually a recorder, which handles command list generation from the application.
// This list is then processed by specific platforms without needing to do too many 
// vtable lookups per drawcall.
// The command list is not thread safe, and should be reset after it is submitted to the device for execution.
class RecluseRenderApi_PUBLIC_API CommandList
{
public:

    typedef U32 Id;
    static const Id kBadId = ~0;

    enum CommandInstance { Primary, Bundle };

    CommandList(CommandInstance instance = Primary);

    void begin();
    void end();

    void clearRenderTarget(uint renderTargetIndex, F32 clearColor[4], const Rect& rect);
    void clearDepthStencil(ClearFlags clearFlags, F32 clearDepth, U8 clearStencil, const Rect& rect); 

    void transitionResources(ResourceTransition* transitions, uint numTransitions);
    void transition(Resource* resource, ResourceState newState);

    void drawIndexedInstanced(uint indexCount, uint instanceCount, uint firstIndex, I32 baseVertex, uint firstInstance);
    void drawInstanced(uint vertexCount, uint instanceCount, uint baseVertex, uint baseInstance);
    void dispatch(U32 x, U32 y, U32 z);

    // Bind render targets to the command list. This will bind the render targets to the command list, and will also bind the depth stencil if provided.
    void bindRenderTargets(ResourceViewId* renderTargets, U32 numRenderTargets, ResourceViewId depthStencil = 0);

    // Begin a render pass. This will begin a render pass, and will also bind the render targets and depth stencil if provided.
    void beginRenderPass();
    void endRenderPass();

    void bindPipeline(Pipeline* pipeline);
    void bindResourceTable(void* resourceTablePtr, uint sizeBytes); // Binds a resource table to the command list.
    void bindSamplerTable(void* samplerTablePtr, uint sizeBytes);  // Binds a sampler table to the command list.

    void executeBundles(CommandList** bundles, U32 numBundles);

    ResourceViewId allocateConstantBuffer(void* constBufData, U32 size);
    
    // Allocate a resource table for the command list. This is a table of resources that can be bound to the command list.
    // The allocation is temporary, and will be freed when the command list is reset. The resource table is used to bind resources to the command list.
    template<typename ResourceTableType>
    ResourceTableType* allocateResourceTable(U32 numResources)
    {
        ResourceTableType* table = m_resourceAllocator.allocate<ResourceTableType>(1);
        return table;
    }

    // Sampler table is a table of samplers that can be bound to the command list.
    // The allocation is temporary, and will be freed when the command list is reset. The sampler table is used to bind samplers to the command list.
    template<typename SamplerTableType>
    SamplerTableType* allocateSamplerTable(U32 numSamplers)
    {
        SamplerTableType* table = m_resourceAllocator.allocate<SamplerTableType>(1);
        return table;
    }
    
    // Reset the command list, and free all resources allocated by the command list.
    void reset();

    Id getId() const { return m_id; }

    // Gets the stream chunk to process. This is the raw bytes of the stream consisting of 
    // all commands that have been recorded.
    CommandStreamChunk getChunk() const;

private:
    // 256 KB is a good preinitial size, and should be cautiously used for mainly
    // drawcalls. If we exceed so much, it is better to optimize the game itself, in order 
    // to reduce these calls. (batching would be highly beneficial.)
    LinearScratchMemory<R_KB(256), true> m_commandAllocator;

    // Resource allocator for managing resources within the command list. This is static, for any resizing
    // we will need to reallocate the resource allocator as well. (Any existing handles
    // will be invalidated if it resizes.)
    LinearScratchMemory<R_KB(256)> m_resourceAllocator;

    // Chunk defines the overall size of the command list.
    CommandStreamChunk m_chunk;

    // Render command list id.
    Id m_id;

    // This Instance
    CommandInstance instance;
};
} // RenderApi
} // Recluse
#endif // RECLUSE_RENDER_API_RENDER_COMMANDLIST_HPP