#ifndef RECLUSE_RENDER_API_INSTANCE_HPP
#define RECLUSE_RENDER_API_INSTANCE_HPP

#pragma once

#include "RenderCommon.hpp"

namespace Recluse {
namespace RenderApi {

// Instance
class Instance
{
public:
    // Create an instance with the given api, returns nullptr if the instance doesn't exist, or not supported.
    static Instance* create(Api api);

    
private:
};
} // RenderApi
} // Recluse
#endif // RECLUSE_RENDER_API_INSTANCE_HPP