#ifndef RECLUSE_RENDER_API_RENDER_DEVICE_HPP
#define RECLUSE_RENDER_API_RENDER_DEVICE_HPP

#include <Recluse/Types.hpp>

#include <Recluse/RenderApi/Common.hpp>
#include <Recluse/RenderApi/Swapchain.hpp>
#include <Recluse/RenderApi/Pipeline.hpp>
#include <Recluse/RenderApi/Resource.hpp>
#include <Recluse/RenderApi/Queue.hpp>

namespace Recluse {
namespace RenderApi {

class Queue;

// Device is the logical instance that is created by the physical device itself (the adapter.)
class Device
{
public:
    struct Description
    {
        B32 enableMeshShaders : 1;
        B32 enableRayTracing : 1;
        B32 enableVariableRateShading : 1;
        B32 enableSamplerFeedback : 1;
        B32 reserved : 26;
    };

    virtual ~Device() { }
    
    virtual ResultCode createResource(const Resource::Description& description, Resource** pResourceOut, 
        void* pInitialData = nullptr, uint initialSizeBytes = 0) = 0;
    virtual ResultCode createQueue(const Queue::Description& description, Queue** ppQueueOut) = 0;
    virtual ResultCode createPipeline(const PipelineDescription& description, Pipeline** pPipelineOut) = 0;

    // Create a swapchain for presentation. Requires a created queue for this.
    virtual ResultCode createSwapchain(const SwapchainCreateDescription& description, Swapchain** ppSwapchainOut) = 0;

    virtual ResultCode freeSwapchain(Swapchain* swapchain) = 0;
    virtual ResultCode freePipeline(Pipeline* pipeline) = 0;
    virtual ResultCode freeQueue(Queue* queue) = 0;
};
} // RenderApi
} // Recluse
#endif // RECLUSE_RENDER_API_RENDER_DEVICE_HPP