//
#include <RenderCommandList.hpp>

#include <Recluse/Threading/Threading.hpp>

namespace Recluse {
namespace RenderApi {

static RenderCommandList::Id kIdCounter = 0;
static MutexGuard idCounterGuard("RenderCommandListMutexCounter");

static RenderCommandList::Id getId()
{
    // Get the id.
    ScopedLock _(idCounterGuard);
    return kIdCounter++;
}

RenderCommandList::RenderCommandList()
    : m_id(kBadId)
{
    m_id = getId();
}

void RenderCommandList::begin()
{
    Command* command = m_commandAllocator.allocate<Command>();
    command->type = CommandType_Begin;
}


void RenderCommandList::end()
{
    Command* command = m_commandAllocator.allocate<Command>();
    command->type = CommandType_End;
}


void RenderCommandList::reset()
{
    m_commandAllocator.clear();
}
} // RenderApi
} // Recluse