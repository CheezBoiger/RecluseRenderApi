#ifndef RECLUSE_RENDER_API_RENDER_SWAPCHAIN_HPP
#define RECLUSE_RENDER_API_RENDER_SWAPCHAIN_HPP

#pragma once

#include <Recluse/Types.hpp>

#include <Recluse/RenderApi/Common.hpp>
#include <Recluse/RenderApi/Queue.hpp>

namespace Recluse {
namespace RenderApi {

class Resource;

class Swapchain : public IApiObject
{
public:
    enum PresentMode
    {
        PresentMode_Immediate,
        PresentMode_NoVSync = PresentMode_Immediate,
        // Vertical Synchronization
        PresentMode_VSync,
        PresentMode_DoubleBuffering = PresentMode_VSync,
        PresentMode_TripleBuffering
    };

    struct Description
    {
        uint                renderWidth;    // The width of the swapchain image.
        uint                renderHeight;   // The height of the swapchain image.
        ResourceFormat      format;         // Format for the swapchain.
        ColorSpace          colorSpace;     // Desired color space.
        uint                numFrames;      // The number of swapchain images/frames.
        WindowHandle        windowHandle;   // The window handle that the swapchain will be presenting to.
        ResourceUsageFlags  usage;          // Swapchain usages, if they are supported.
        PresentMode         presentMode;
    };

    virtual ResultCode                  rebuild(const Description& description) = 0;
    // Get the current backbuffer.
    virtual Resource*                   currentBackbuffer() = 0;
    virtual Description                 getDescription() = 0;
};
} // RenderApi
} // Recluse
#endif // RECLUSE_RENDER_API_RENDER_SWAPCHAIN_HPP