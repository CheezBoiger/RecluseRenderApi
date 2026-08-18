#ifndef RECLUSE_RENDER_API_RENDER_QUEUE_HPP
#define RECLUSE_RENDER_API_RENDER_QUEUE_HPP

#pragma once 

#include <Recluse/RenderApi/Common.hpp>
#include <Recluse/RenderApi/CommandList.hpp>

#include <Recluse/Types.hpp>

namespace Recluse {
namespace RenderApi {

typedef u32 Fence;

struct SubmitDescription
{
    Fence*          pWaitFences;        // Fences to wait on before executing the command list.
    U32             numWaitFences;      // Number of fences to wait on.
    Fence*          pSignalFences;      // Fences to signal after executing the command list.
    U32             numSignalFences;    // Number of fences to signal after executing the command list.
    CommandList**   pCommandLists; // Command lists to submit to the queue.
    U32             numCommandLists;    // Number of command lists to submit to the queue.
};

class Queue
{
public:
    // Submit the render command list.
    virtual RecluseResult submit(U32 numLists, const CommandList** commandlists) = 0;

    // Wait for device until it is idle
    virtual RecluseResult waitIdle() = 0;

private:
    
};
} // RenderApi
} // Recluse
#endif // RECLUSE_RENDER_API_RENDER_QUEUE_HPP