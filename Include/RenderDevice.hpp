#ifndef RECLUSE_RENDER_API_RENDER_DEVICE_HPP
#define RECLUSE_RENDER_API_RENDER_DEVICE_HPP

#include <Recluse/Types.hpp>

namespace Recluse {
namespace RenderApi {


// Device is the logical instance that is created by the physical device itself (the adapter.)
class IDevice
{
public:
    virtual ~IDevice() { }
    
    virtual ResultCode createResource() = 0;
    virtual ResultCode createQueue() = 0;
    virtual ResultCode createSwapchain() = 0;
};
} // RenderApi
} // Recluse
#endif // RECLUSE_RENDER_API_RENDER_DEVICE_HPP