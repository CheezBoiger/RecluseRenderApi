//
#include <Recluse/RenderApi/CommandList.hpp>

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
{
    m_id = getId();
}

void CommandList::begin()
{
    BeginCommand* command = m_commandAllocator.allocate<BeginCommand>();
    command->type = CommandOpcode_Begin;
}


void CommandList::end()
{
    Command* command = m_commandAllocator.allocate<Command>();
    command->type = CommandOpcode_End;
}


void CommandList::dispatch(U32 x, U32 y, U32 z)
{
    DispatchCommand* command = m_commandAllocator.allocate<DispatchCommand>();
    command->type = CommandOpcode_Dispatch;
    command->x = x;
    command->y = y;
    command->z = z;
}


void CommandList::bindResourceTable(void* resourceTablePtr, uint sizeBytes)
{
    BindResourceTableCommand* command = m_commandAllocator.allocate<BindResourceTableCommand>();
    command->type = CommandOpcode_BindResourceTable;
    command->resourceTablePtr = reinterpret_cast<UPtr>(resourceTablePtr);
    command->sizeBytes = sizeBytes;
}

void CommandList::bindSamplerTable(void*samplerTablePtr, uint sizeBytes)
{
    BindSamplerTableCommand* command = m_commandAllocator.allocate<BindSamplerTableCommand>();
    command->type = CommandOpcode_BindSamplerTable;
    command->samplerTablePtr = reinterpret_cast<UPtr>(samplerTablePtr);
    command->sizeBytes = sizeBytes;
}

void CommandList::bindPipeline(PipelineId pipeline)
{
    BindPipelineCommand* command = m_commandAllocator.allocate<BindPipelineCommand>();
    command->type = CommandOpcode_BindPipeline;
    command->pipeline = pipeline;
}

void CommandList::reset()
{
    m_commandAllocator.clear();
    m_resourceAllocator.clear();
}
} // RenderApi
} // Recluse