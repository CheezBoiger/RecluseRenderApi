//
#include <Recluse/RenderApi/CommandList.hpp>
#include <Recluse/RenderApi/Pipeline.hpp>
#include <Recluse/Threading/Threading.hpp>
#include <Recluse/RenderApi/Resource.hpp>

#include "Shared/CommandOps.hpp"

namespace Recluse {
namespace RenderApi {

static CommandList::Id kIdCounter = 0;
static MutexGuard idCounterGuard("RenderCommandListMutexCounter");

static CommandList::Id getId()
{
    // Get the id.
    ScopedLock _(idCounterGuard);
    return kIdCounter++;
}

CommandList::CommandList()
    : m_id(kBadId)
{
    m_id = getId();
}

void CommandList::begin(const CommandList::BeginDescription& beginDescription)
{
    CommandHeader* command = (CommandHeader*)m_commandAllocator.allocate<CommandHeader>();
    command->opcode = CommandOpcode_Begin;
    command->size = 0;

    // Make a chunk and store for processing.
    CommandStreamChunk chunk;
    chunk.baseAddress   = (UPtr)command;
    chunk.sizeBytes     = sizeof(CommandHeader);
    chunk.instance      = beginDescription.instance;
    chunk.type          = beginDescription.type;
    chunk.id            = m_id;

    m_chunks.push_back(chunk);
}

void CommandList::end()
{
    CommandHeader* command = m_commandAllocator.allocate<CommandHeader>();
    command->opcode = CommandOpcode_End;
    command->size = 0;

    m_chunks.back().sizeBytes += sizeof(CommandHeader);
}

void CommandList::dispatch(U32 x, U32 y, U32 z)
{
    CommandHeader* header = (CommandHeader*)m_commandAllocator.allocateRaw(CommandHeader::dataSize<DispatchCommand>());
    header->opcode = CommandOpcode_Dispatch;
    header->size = sizeof(DispatchCommand);

    DispatchCommand* command = CommandHeader::dataOffset<DispatchCommand>(header);
    command->x = x;
    command->y = y;
    command->z = z;

    m_chunks.back().sizeBytes += CommandHeader::dataSize<DispatchCommand>();
}

void CommandList::bindResourceTable(void* resourceTablePtr, uint sizeBytes)
{
    CommandHeader* header = (CommandHeader*)m_commandAllocator.allocateRaw(CommandHeader::dataSize<BindResourceTableCommand>());
    header->opcode = CommandOpcode_BindResourceTable;
    header->size = sizeof(BindResourceTableCommand);

    BindResourceTableCommand* command = CommandHeader::dataOffset<BindResourceTableCommand>(header);
    command->resourceTablePtr = reinterpret_cast<UPtr>(resourceTablePtr);
    command->sizeBytes = sizeBytes;

    m_chunks.back().sizeBytes += CommandHeader::dataSize<BindResourceTableCommand>();
}

void CommandList::bindSamplerTable(void*samplerTablePtr, uint sizeBytes)
{
    CommandHeader* header = (CommandHeader*)m_commandAllocator.allocateRaw(CommandHeader::dataSize<BindSamplerTableCommand>());
    header->opcode = CommandOpcode_BindSamplerTable;
    header->size = sizeof(BindSamplerTableCommand);

    BindSamplerTableCommand* command = CommandHeader::dataOffset<BindSamplerTableCommand>(header);
    command->samplerTablePtr = reinterpret_cast<UPtr>(samplerTablePtr);
    command->sizeBytes = sizeBytes;

    m_chunks.back().sizeBytes += CommandHeader::dataSize<BindSamplerTableCommand>();
}

void CommandList::bindPipeline(Pipeline* pipeline)
{
    CommandHeader* header = (CommandHeader*)m_commandAllocator.allocateRaw(CommandHeader::dataSize<BindPipelineCommand>());
    header->opcode = CommandOpcode_BindPipeline;
    header->size = sizeof(BindPipelineCommand);

    BindPipelineCommand* command = CommandHeader::dataOffset<BindPipelineCommand>(header);
    command->pipeline = pipeline->getId();

    m_chunks.back().sizeBytes += CommandHeader::dataSize<BindPipelineCommand>();
}

void CommandList::bindRenderTargets(ResourceViewId* renderTargets, uint numRenderTargets, ResourceViewId depthStencil)
{
    const Bool hasDepthStencil = (depthStencil != 0 ? 1 : 0);

    const uint dataSizeBytes = sizeof(BindRenderTargetsHeader) +
        sizeof(ResourceViewId) * (numRenderTargets) +
        sizeof(ResourceViewId) * hasDepthStencil;

    const uint sizeBytes = sizeof(CommandHeader) + dataSizeBytes;

    CommandHeader* header = (CommandHeader*)m_commandAllocator.allocateRaw(sizeBytes);
    header->opcode = CommandOpcode_BindRenderTargets;
    header->size = dataSizeBytes;

    BindRenderTargetsHeader* renderTargetsHeader = CommandHeader::dataOffset<BindRenderTargetsHeader>(header);
    renderTargetsHeader->numRenderTargets = numRenderTargets;
    renderTargetsHeader->hasDepthStencil = hasDepthStencil;

    ResourceViewId* viewId = reinterpret_cast<ResourceViewId*>((UPtr)renderTargetsHeader + sizeof(BindRenderTargetsHeader));
    for (uint i = 0; i < numRenderTargets; ++i)
    {
        *viewId = renderTargets[i];
        ++viewId;
    }

    if (hasDepthStencil)
    {
        *viewId = depthStencil;
    }

    m_chunks.back().sizeBytes += sizeBytes;
}

void CommandList::drawIndexedInstanced(uint indexCount, uint instanceCount, uint firstIndex, I32 baseVertex, uint firstInstance)
{
    CommandHeader* header = (CommandHeader*)m_commandAllocator.allocateRaw(CommandHeader::dataSize<DrawIndexedInstancedCommand>());
    header->opcode = CommandOpcode_DrawIndexedInstanced;
    header->size = sizeof(DrawIndexedInstancedCommand);

    DrawIndexedInstancedCommand* command = CommandHeader::dataOffset<DrawIndexedInstancedCommand>(header);
    command->baseVertex = baseVertex;
    command->indexCount = indexCount;
    command->instanceCount = instanceCount;
    command->startIndex = firstIndex;
    command->startInstance = firstInstance;

    m_chunks.back().sizeBytes += CommandHeader::dataSize<DrawIndexedInstancedCommand>();
}

void CommandList::drawInstanced(uint vertexCount, uint instanceCount, uint baseVertex, uint baseInstance)
{
    CommandHeader* header = (CommandHeader*)m_commandAllocator.allocateRaw(CommandHeader::dataSize<DrawInstancedCommand>());
    header->opcode = CommandOpcode_DrawInstanced;
    header->size = sizeof(DrawInstancedCommand);

    DrawInstancedCommand* command = CommandHeader::dataOffset<DrawInstancedCommand>(header);
    command->baseInstance = baseInstance;
    command->baseVertex = baseVertex;
    command->instanceCount = instanceCount;
    command->vertexCount = vertexCount;
    
    m_chunks.back().sizeBytes += CommandHeader::dataSize<DrawInstancedCommand>();
}

void CommandList::transitionResources(ResourceTransition* transitions, uint numTransitions)
{
    if (numTransitions == 0) return;
    const uint dataSizeBytes = sizeof(BarrierTransitionHeader) + sizeof(Transition) * numTransitions;
    const uint sizeBytes = sizeof(CommandHeader) + 
        dataSizeBytes;
    
    CommandHeader* header = (CommandHeader*)m_commandAllocator.allocateRaw(sizeBytes);
    header->opcode = CommandOpcode_BarrierTransition;
    header->size = dataSizeBytes;

    BarrierTransitionHeader* barrierHeader = CommandHeader::dataOffset<BarrierTransitionHeader>(header);
    barrierHeader->numTransitions = numTransitions;
    
    UPtr offset = reinterpret_cast<UPtr>(barrierHeader) + sizeof(BarrierTransitionHeader);

    for (uint i = 0; i < numTransitions; ++i)
    {
        Transition* transition = reinterpret_cast<Transition*>(offset + sizeof(Transition) * i);
        transition->resource = transitions[i].resource;
        transition->resourceState = transitions[i].newState;
    }
    
    m_chunks.back().sizeBytes += sizeBytes; 
}

void CommandList::transition(Resource* resource, ResourceState resourceState)
{
    ResourceTransition trans;
    trans.newState = resourceState;
    trans.resource = resource;
    transitionResources(&trans, 1);
}

void CommandList::executeBundles(CommandList** bundles, uint numBundles)
{
    if (numBundles == 0) return;

    const uint dataSizeBytes = sizeof(BundlesHeader)
        + sizeof(CommandStreamChunk) * numBundles;
    const uint sizeBytes = sizeof(CommandHeader) + dataSizeBytes; 

    CommandHeader* header = (CommandHeader*)m_commandAllocator.allocateRaw(sizeBytes);
    header->opcode = CommandOpcode_ExecuteBundles;
    header->size = dataSizeBytes;

    BundlesHeader* bundleHeader = CommandHeader::dataOffset<BundlesHeader>(header);
    bundleHeader->numBundles = numBundles;
    
    UPtr offset = reinterpret_cast<UPtr>(bundleHeader) + sizeof(BundlesHeader);

    for (uint i = 0; i < numBundles; ++i)
    {
        const CommandStreamChunk* bundleChunks = bundles[i]->getChunks();
        const uint numChunks = bundles[i]->getNumChunks();

        CommandStreamChunk* chunk = reinterpret_cast<CommandStreamChunk*>(offset + sizeof(CommandStreamChunk) * i);
        for (uint j = 0; j < numChunks; ++j)
        {
            chunk[j] = bundleChunks[j]; 
        }
    }
    
    m_chunks.back().sizeBytes += sizeBytes;
}

void CommandList::clearRenderTarget(uint renderTargetIndex, const F32 clearColor[4], const Rect& rect)
{
    CommandHeader* header = (CommandHeader*)m_commandAllocator.allocateRaw(CommandHeader::dataSize<ClearRenderTargetHeader>());
    header->opcode = CommandOpcode_ClearRenderTarget;
    header->size = sizeof(ClearRenderTargetHeader);
    
    ClearRenderTargetHeader* clearRenderTargetHeader = CommandHeader::dataOffset<ClearRenderTargetHeader>(header);
    clearRenderTargetHeader->clearColor[0] = clearColor[0];
    clearRenderTargetHeader->clearColor[1] = clearColor[1];
    clearRenderTargetHeader->clearColor[2] = clearColor[2];
    clearRenderTargetHeader->clearColor[3] = clearColor[3];
    
    clearRenderTargetHeader->rect = rect;
    clearRenderTargetHeader->renderTargetIndex = renderTargetIndex;

    m_chunks.back().sizeBytes += CommandHeader::dataSize<ClearRenderTargetHeader>();
}

void CommandList::clearDepthStencil(ClearFlags clearFlags, F32 clearDepth, U8 clearStencil, const Rect& rect)
{
    CommandHeader* header = (CommandHeader*)m_commandAllocator.allocateRaw(CommandHeader::dataSize<ClearDepthStencilHeader>());
    header->opcode = CommandOpcode_ClearDepthStencil;
    header->size = sizeof(ClearDepthStencilHeader);

    ClearDepthStencilHeader* clearDepthStencilHeader = CommandHeader::dataOffset<ClearDepthStencilHeader>(header);
    clearDepthStencilHeader->rect = rect;
    clearDepthStencilHeader->clearFlags = clearFlags;
    clearDepthStencilHeader->clearDepth = clearDepth;
    clearDepthStencilHeader->clearStencil = clearStencil;

    m_chunks.back().sizeBytes += CommandHeader::dataSize<ClearDepthStencilHeader>();
}

void CommandList::reset()
{
    m_commandAllocator.clear();
    m_resourceAllocator.clear();
    m_chunks.clear();
}


const CommandStreamChunk* CommandList::getChunks() const
{
    return m_chunks.data();
}
} // RenderApi
} // Recluse