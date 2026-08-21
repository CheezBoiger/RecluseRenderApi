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

CommandList::CommandList(CommandInstance instance)
    : m_id(kBadId)
    , instance(instance)
{
    m_id = getId();
}

void CommandList::begin()
{
    CommandHeader* command = m_commandAllocator.allocate<CommandHeader>();
    command->opcode = CommandOpcode_Begin;
    command->size = sizeof(CommandHeader);

    m_chunk.baseAddress = (UPtr)command;
    m_chunk.sizeBytes += sizeof(CommandHeader);
}

void CommandList::end()
{
    CommandHeader* command = m_commandAllocator.allocate<CommandHeader>();
    command->opcode = CommandOpcode_End;
    command->size = sizeof(CommandHeader);

    m_chunk.sizeBytes += sizeof(CommandHeader);
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

    m_chunk.sizeBytes += CommandHeader::dataSize<DispatchCommand>();
}

void CommandList::bindResourceTable(void* resourceTablePtr, uint sizeBytes)
{
    CommandHeader* header = (CommandHeader*)m_commandAllocator.allocateRaw(CommandHeader::dataSize<BindResourceTableCommand>());
    header->opcode = CommandOpcode_BindResourceTable;
    header->size = sizeof(BindResourceTableCommand);

    BindResourceTableCommand* command = CommandHeader::dataOffset<BindResourceTableCommand>(header);
    command->resourceTablePtr = reinterpret_cast<UPtr>(resourceTablePtr);
    command->sizeBytes = sizeBytes;

    m_chunk.sizeBytes += CommandHeader::dataSize<BindResourceTableCommand>();
}

void CommandList::bindSamplerTable(void*samplerTablePtr, uint sizeBytes)
{
    CommandHeader* header = (CommandHeader*)m_commandAllocator.allocateRaw(CommandHeader::dataSize<BindSamplerTableCommand>());
    header->opcode = CommandOpcode_BindSamplerTable;
    header->size = sizeof(BindSamplerTableCommand);

    BindSamplerTableCommand* command = CommandHeader::dataOffset<BindSamplerTableCommand>(header);
    command->samplerTablePtr = reinterpret_cast<UPtr>(samplerTablePtr);
    command->sizeBytes = sizeBytes;

    m_chunk.sizeBytes += CommandHeader::dataSize<BindSamplerTableCommand>();
}

void CommandList::bindPipeline(Pipeline* pipeline)
{
    CommandHeader* header = (CommandHeader*)m_commandAllocator.allocateRaw(CommandHeader::dataSize<BindPipelineCommand>());
    header->opcode = CommandOpcode_BindPipeline;
    header->size = sizeof(BindPipelineCommand);

    BindPipelineCommand* command = CommandHeader::dataOffset<BindPipelineCommand>(header);
    command->pipeline = pipeline->getId();

    m_chunk.sizeBytes += CommandHeader::dataSize<BindPipelineCommand>();
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

    m_chunk.sizeBytes += sizeBytes;
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

    m_chunk.sizeBytes += CommandHeader::dataSize<DrawIndexedInstancedCommand>();
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
    
    m_chunk.sizeBytes += CommandHeader::dataSize<DrawInstancedCommand>();
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
        transition->id = transitions[i].resource->getId();
        transition->resourceState = transitions[i].newState;
    }
    
    m_chunk.sizeBytes += sizeBytes; 
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
        CommandStreamChunk* chunk = reinterpret_cast<CommandStreamChunk*>(offset + sizeof(CommandStreamChunk) * i);
        *chunk = bundles[i]->getChunk();
    }
    
    m_chunk.sizeBytes += sizeBytes;
}

void CommandList::reset()
{
    m_commandAllocator.clear();
    m_resourceAllocator.clear();
    m_chunk = { };
}


CommandStreamChunk CommandList::getChunk() const
{
    return m_chunk;
}
} // RenderApi
} // Recluse