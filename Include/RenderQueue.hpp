#ifndef RECLUSE_RENDER_API_RENDER_QUEUE_HPP
#define RECLUSE_RENDER_API_RENDER_QUEUE_HPP

#pragma once 

#include <RenderCommon.hpp>
#include <RenderCommandList.hpp>

#include <Recluse/Types.hpp>

namespace Recluse {
namespace RenderApi {
class Queue
{
public:
    // Process the render command list.
    RecluseResult enqueue(U32 numLists, const CommandList** commandlists);

    // Wait for device to be waited on
    RecluseResult wait();

private:
    
};
} // RenderApi
} // Recluse
#endif // RECLUSE_RENDER_API_RENDER_QUEUE_HPP