//
#include <Recluse/RenderApi/Device.hpp>

#include "Shared/CommandOps.hpp"
#include "Shared/ResourceState.hpp"

namespace Recluse {
namespace RenderApi {

ResultCode FrameProcess::submitCommandLists(CommandQueueType type, CommandList** lists, uint numCommandLists)
{
    for (uint i = 0; i < numCommandLists; ++i)
    {
        if (!lists[i]) return RecluseResult_NullPtrExcept;
        // Processing initial transitions for initial resources.
        const std::vector<UPtr>& initialTransitions = lists[i]->m_initialTransitions;
        for (uint j = 0; j < initialTransitions.size(); ++j)
        {
            Transition* transition = (Transition*)initialTransitions[j];
            if (!transition) continue;
            Resource* resource = transition->resource;
            if (!resource) continue;
            ResourceState globalCurrentState = Database::obtainResourceState(resource->getId());
            auto it = lists[i]->m_localResourceStateMap.find(resource->getId().value);
            if (it != lists[i]->m_localResourceStateMap.end())
            {
                transition->oldState = globalCurrentState;
            }
        }

        // Update the global resource state for all resources in the command list.
        // This is done in order of the command list input (from first to last.) So it assumes 
        // the command lists are executed in-order, not out-of-order.
        const std::unordered_map<uint, LocalResourceState>& localResourceStateMap = lists[i]->m_localResourceStateMap;
        for (auto& it : localResourceStateMap)
        {
            ResourceId resourceId = { it.first };
            Database::setResourceState(resourceId, it.second.currentState);
        }
    }

    return onSubmitCommandLists(type, lists, numCommandLists);
}
} // RenderApi
} // Recluse