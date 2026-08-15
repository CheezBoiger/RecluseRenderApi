#ifndef RECLUSE_RENDER_API_RENDER_SWAPCHAIN_HPP
#define RECLUSE_RENDER_API_RENDER_SWAPCHAIN_HPP

#pragma once

#include <Recluse/Types.hpp>

#include <RenderQueue.hpp>

namespace Recluse {
namespace RenderApi {

class Swapchain
{
public:
    
    // Submit a present to the given queue.
    virtual ResultCode present() = 0;

private:
    
};
} // RenderApi
} // Recluse
#endif // RECLUSE_RENDER_API_RENDER_SWAPCHAIN_HPP