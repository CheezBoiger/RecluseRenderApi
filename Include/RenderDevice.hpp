#ifndef RECLUSE_RENDER_API_RENDER_DEVICE_HPP
#define RECLUSE_RENDER_API_RENDER_DEVICE_HPP

#include <Recluse/Types.hpp>
#include <RenderCommon.hpp>
#include <RenderPipeline.hpp>

namespace Recluse {
namespace RenderApi {

class Queue;
class Swapchain;
class Resource;

struct SwapchainCreateDescription
{
    typedef void* WindowHandle;

    Queue*          pQueue;         // The queue that the swapchain instance will be referencing for presentation.
    uint            renderWidth;    // The width of the swapchain image.
    uint            renderHeight;   // The height of the swapchain image.
    uint            format;         // Format for the swapchain.
    uint            numFrames;
    WindowHandle    windowHandle;    // The window handle that the swapchain will be presenting to.
};

// Device is the logical instance that is created by the physical device itself (the adapter.)
class Device
{
public:
    virtual ~Device() { }
    
    virtual ResultCode createResource(const ResourceDescription& description, Resource* pResourceOut) = 0;
    virtual ResultCode createQueue() = 0;
    virtual ResultCode createPipeline(const PipelineDescription& description, PipelineId* pPipelineOut) = 0;

    // Create a swapchain for presentation. Requires a created queue for this.
    virtual Swapchain* createSwapchain(const SwapchainCreateDescription& description) = 0;

    virtual ResultCode freeSwapchain(Swapchain* swapchain) = 0;
    virtual ResultCode freePipeline(PipelineId pipeline) = 0;
};
} // RenderApi
} // Recluse
#endif // RECLUSE_RENDER_API_RENDER_DEVICE_HPP