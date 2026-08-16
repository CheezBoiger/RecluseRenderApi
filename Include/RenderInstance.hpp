#ifndef RECLUSE_RENDER_API_INSTANCE_HPP
#define RECLUSE_RENDER_API_INSTANCE_HPP

#pragma once

#include "RenderCommon.hpp"

namespace Recluse {
namespace RenderApi {

class Adapter;

// Instance is like a factory for the render api. It is intented to 
// be used to create the render api, and enumerate adapters for the given api.
class Instance
{
public:
    // Create an instance with the given api, returns nullptr if the instance doesn't exist, or not supported.
    static Instance* create(Api api);

    u32 enumerateAdapters(Adapter** adapters, u32 maxAdapters);
private:
};
} // RenderApi
} // Recluse
#endif // RECLUSE_RENDER_API_INSTANCE_HPP