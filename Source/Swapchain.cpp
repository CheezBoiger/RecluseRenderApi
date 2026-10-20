//
#include <Recluse/RenderApi/Swapchain.hpp>

#include "Shared/ResourceState.hpp"

namespace Recluse {
namespace RenderApi {

ResultCode Swapchain::rebuild(const Swapchain::Description& description)
{
    ResultCode result = onRebuild(description);
    if (result == RecluseResult_Ok)
    {
        for (uint idx = 0; idx < description.numFrames; ++idx)
        {
            Resource* swapchainResource = backBuffer(idx);
            R_ASSERT(swapchainResource != nullptr);
            
            Database::setResourceState(swapchainResource->getId(), ResourceState_Unknown);
        }
    }
    return result;
}
} // RenderApi
} // Recluse