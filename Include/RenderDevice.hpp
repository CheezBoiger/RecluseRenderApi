#ifndef RECLUSE_RENDER_API_RENDER_DEVICE_HPP
#define RECLUSE_RENDER_API_RENDER_DEVICE_HPP

#include <Recluse/Types.hpp>

namespace Recluse {
namespace RenderApi {

class Queue;
class Swapchain;

struct SwapchainCreateDescription
{
    Queue*  pQueue;         // The queue that the swapchain instance will be referencing for presentation.
    uint    renderWidth;    // The width of the swapchain image.
    uint    renderHeight;   // The height of the swapchain image.
    uint    format;         // Format for the swapchain.
    uint    numFrames;
};

// Device is the logical instance that is created by the physical device itself (the adapter.)
class Device
{
public:
    virtual ~Device() { }
    
    virtual ResultCode createResource() = 0;
    virtual ResultCode createQueue() = 0;

    // Create a swapchain for presentation. Requires a created queue for this.
    virtual Swapchain* createSwapchain(const SwapchainCreateDescription& description) = 0;

    virtual ResultCode freeSwapchain(Swapchain* swapchain) = 0;
};
} // RenderApi
} // Recluse
#endif // RECLUSE_RENDER_API_RENDER_DEVICE_HPP