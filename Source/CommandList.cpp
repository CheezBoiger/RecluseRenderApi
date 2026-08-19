//
#include <Recluse/RenderApi/CommandList.hpp>
#include <Recluse/RenderApi/Pipeline.hpp>
#include <Recluse/Threading/Threading.hpp>

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
    , instance(CommandInstance::Primary)
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
    uint sizeBytes = sizeof(CommandHeader) + sizeof(BindRenderTargetsHeader) +
        sizeof(ResourceViewId) * (numRenderTargets) +
        sizeof(ResourceViewId) * hasDepthStencil;

    CommandHeader* header = (CommandHeader*)m_commandAllocator.allocateRaw(sizeBytes);
    header->opcode = CommandOpcode_BindRenderTargets;
    header->size = sizeBytes;

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