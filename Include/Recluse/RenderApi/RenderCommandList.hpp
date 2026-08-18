#ifndef RECLUSE_RENDER_API_RENDER_COMMANDLIST_HPP
#define RECLUSE_RENDER_API_RENDER_COMMANDLIST_HPP

#pragma once

#include <Recluse/RenderApi/RenderCommon.hpp>

#include <Recluse/Types.hpp>
#include <Recluse/Memory/LinearScratchMemory.hpp>

#include <RecluseRenderApi_exports.hpp>

namespace Recluse {
namespace RenderApi {

// CommandList is actually a recorder, which handles command list generation from the application.
// This list is then processed by specific platforms without needing to do too many 
// vtable lookups per drawcall.
class RecluseRenderApi_PUBLIC_API CommandList
{
public:
    typedef U32 Id;
    static const Id kBadId = ~0;

    enum CommandInstance { Instance_Static, Instance_Dynamic };

    CommandList();
    CommandList(const CommandList&);
    CommandList(const CommandList&&);

    void begin();
    void end();

    void drawIndexedInstanced();
    void drawInstanced();
    void dispatch(U32 x, U32 y, U32 z);

    // Bind render targets to the command list. This will bind the render targets to the command list, and will also bind the depth stencil if provided.
    void bindRenderTargets(ResourceViewId* renderTargets, U32 numRenderTargets, ResourceViewId depthStencil);

    // Begin a render pass. This will begin a render pass, and will also bind the render targets and depth stencil if provided.
    void beginRenderPass();
    void endRenderPass();

    void bindPipeline(PipelineId pipeline);
    void bindResourceTable(void* resourceTablePtr, uint sizeBytes); // Binds a resource table to the command list.
    void bindSamplerTable(void* samplerTablePtr, uint sizeBytes);  // Binds a sampler table to the command list.

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

private:
    // Render command list id.
    Id m_id;

    // This Instance
    CommandInstance instance;

    // 1 Megabyte is a good preinitial size, and should be cautiously used for mainly
    // drawcalls. If we exceed so much, it is better to optimize the game itself, in order 
    // to reduce these calls. (batching would be highly beneficial.)
    LinearScratchMemory<R_MB(1), true> m_commandAllocator;

    // Resource allocator for managing resources within the command list. This is static, for any resizing
    // we will need to reallocate the resource allocator as well. (Any existing handles
    // will be invalidated if it resizes.)
    LinearScratchMemory<R_MB(1)> m_resourceAllocator;
};
} // RenderApi
} // Recluse
#endif // RECLUSE_RENDER_API_RENDER_COMMANDLIST_HPP