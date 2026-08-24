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
class CommandList;

typedef UPtr FrameHandle;

// FrameProcess is an object that handles the process of a frame, essential for 
// managing the structure of the overall rendering process.
class FrameProcess
{
public:

    struct FrameDescription
    {
        // The swapchain to process.
        Swapchain* swapchain = nullptr;
    };

    struct Description
    {
        // The max number of frames that can be in process on the device.
        uint maxFramesInFlight;

        // The number of threads that can process a certain number of command lists at a time.
        // Usually intended to work in parallel as much as possible.
        uint numCommandListJobThreads;
    };

    // Submit command lists to the frame process, must be called while frame has begun processing.
    virtual ResultCode                  submitCommandLists(CommandQueueType type, CommandList* lists, uint numCommandLists) = 0;

    // Begins a frame to process.
    virtual void                        beginFrame(const FrameDescription& frameDescription) = 0;

    // Ends a frame, and produces a handle output that is ready to be processed. Intended to be executed in the 
    // device queue.
    virtual FrameHandle                 endFrame() = 0;

    // Waits for any fences in the process.
    virtual ResultCode                  waitForFences(Fence* fences, uint numFences) = 0;

    // 
    virtual ResultCode                  signalFences(Fence* fences, uint numFences) = 0;
    
    virtual ResultCode                  waitIdle() = 0;
};

// Device is the logical instance that is created by the physical device itself (the adapter.)
class Device
{
public:
    struct Description
    {
        B32                 enableMeshShaders : 1;
        B32                 enableRayTracing : 1;
        B32                 enableVariableRateShading : 1;
        B32                 enableSamplerFeedback : 1;
        B32                 reserved : 24;
        CommandQueueType*   queueTypes;
        u32                 queueTypeCount;
    };

    virtual ~Device() { }

    virtual FrameProcess*   createFrameProcess(const FrameProcess::Description& description = { }) = 0;
    virtual Resource*       createResource(const Resource::Description& description, void* pInitialData = nullptr, uint initialSizeBytes = 0) = 0;
    virtual Pipeline*       createPipeline(const PipelineDescription& description) = 0;
    virtual Fence           createFence() = 0;

    // Create a swapchain for presentation. Requires a created queue for this.
    virtual Swapchain*      createSwapchain(const Swapchain::Description& description) = 0;

    virtual ResultCode      freeSwapchain(Swapchain* swapchain) = 0;
    virtual ResultCode      freePipeline(Pipeline* pipeline) = 0;

    virtual ResultCode      freeFrameProcess(FrameProcess* frameProcess) = 0;
    virtual ResultCode      freeFence(Fence fence) = 0;
    
    virtual ResultCode      processFrame(FrameHandle frame) = 0;
    
};
} // RenderApi
} // Recluse
#endif // RECLUSE_RENDER_API_RENDER_DEVICE_HPP