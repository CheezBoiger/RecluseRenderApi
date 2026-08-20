#ifndef RECLUSE_RENDER_API_RENDER_SWAPCHAIN_HPP
#define RECLUSE_RENDER_API_RENDER_SWAPCHAIN_HPP

#pragma once

#include <Recluse/Types.hpp>

#include <Recluse/RenderApi/Common.hpp>
#include <Recluse/RenderApi/Queue.hpp>

namespace Recluse {
namespace RenderApi {

class Resource;

struct SwapchainCreateDescription
{
    typedef void* WindowHandle;

    Queue*          pQueue;         // The queue that the swapchain instance will be referencing for presentation.
    uint            renderWidth;    // The width of the swapchain image.
    uint            renderHeight;   // The height of the swapchain image.
    uint            format;         // Format for the swapchain.
    uint            numFrames;      // The number of swapchain images/frames.
    WindowHandle    windowHandle;    // The window handle that the swapchain will be presenting to.
};

class Swapchain
{
public:
    
    // Submit a present to the given queue.
    virtual ResultCode present() = 0;

    virtual ResultCode rebuild(const SwapchainCreateDescription& description) = 0;

    virtual Resource* current() = 0;

    virtual SwapchainCreateDescription getDescription() = 0;

private:
    
};
} // RenderApi
} // Recluse
#endif // RECLUSE_RENDER_API_RENDER_SWAPCHAIN_HPP