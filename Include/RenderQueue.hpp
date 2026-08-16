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
    virtual RecluseResult process(U32 numLists, const CommandList** commandlists) = 0;

    // Wait for device to be waited on
    virtual RecluseResult wait() = 0;

private:
    
};
} // RenderApi
} // Recluse
#endif // RECLUSE_RENDER_API_RENDER_QUEUE_HPP