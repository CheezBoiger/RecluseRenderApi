#ifndef RECLUSE_RENDER_API_RENDER_DEVICE_HPP
#define RECLUSE_RENDER_API_RENDER_DEVICE_HPP

#include <Recluse/Types.hpp>

#include <Recluse/RenderApi/Common.hpp>
#include <Recluse/RenderApi/Swapchain.hpp>
#include <Recluse/RenderApi/Pipeline.hpp>

namespace Recluse {
namespace RenderApi {

class Queue;
class Resource;

// Device is the logical instance that is created by the physical device itself (the adapter.)
class Device
{
public:
    virtual ~Device() { }
    
    virtual ResultCode createResource(const ResourceDescription& description, Resource** pResourceOut) = 0;
    virtual ResultCode createQueue(const QueueCreateDescription& description, Queue** ppQueueOut) = 0;
    virtual ResultCode createPipeline(const PipelineDescription& description, Pipeline** pPipelineOut) = 0;

    // Create a swapchain for presentation. Requires a created queue for this.
    virtual ResultCode createSwapchain(const SwapchainCreateDescription& description, Swapchain** ppSwapchainOut) = 0;

    virtual ResultCode freeSwapchain(Swapchain* swapchain) = 0;
    virtual ResultCode freePipeline(PipelineId pipeline) = 0;
    virtual ResultCode freeQueue(Queue* queue) = 0;
};
} // RenderApi
} // Recluse
#endif // RECLUSE_RENDER_API_RENDER_DEVICE_HPP