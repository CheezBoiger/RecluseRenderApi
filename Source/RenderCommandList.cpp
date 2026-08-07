//
#include <RenderCommandList.hpp>

#include <Recluse/Threading/Threading.hpp>

#include "CommandOps.hpp"

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
    command->type = CommandType_Begin;
}


void CommandList::end()
{
    Command* command = m_commandAllocator.allocate<Command>();
    command->type = CommandType_End;
}


void CommandList::dispatch(U32 x, U32 y, U32 z)
{
    DispatchCommand* command = m_commandAllocator.allocate<DispatchCommand>();
    command->x = x;
    command->y = y;
    command->z = z;
}


void CommandList::reset()
{
    m_commandAllocator.clear();
}
} // RenderApi
} // Recluse