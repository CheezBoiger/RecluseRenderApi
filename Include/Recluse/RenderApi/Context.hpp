#ifndef RECLUSE_RENDER_API_INSTANCE_HPP
#define RECLUSE_RENDER_API_INSTANCE_HPP

#pragma once

#include <Recluse/RenderApi/Common.hpp>
#include <Recluse/RenderApi/Adapter.hpp>

#include <RecluseRenderApi_exports.hpp>

namespace Recluse {
namespace RenderApi {

class Adapter;

// Context is the starting point for the render api. It is intented to 
// be used to create the render api, and enumerate adapters for the given api.
class RecluseRenderApi_PUBLIC_API Context : public IApiObject
{
public:

    struct Description
    {
        const char*     applicationName;
        const char*     engineName;
        Bool32          enableValidation       : 1;
        Bool32          enableDebugMarkers     : 1;
        Bool32          enableApiDump          : 1;
        Bool32          reserved               : 29;
    };

    Context(Api api, uint id) : m_api(api), m_id(id) {}

    virtual Bool        isValid() const override { return false; }

    // Create an instance of a context with the given api, returns nullptr if the instance doesn't exist, or not supported.
    static Context*     create(Api api, const Description& description = Description());
    static ResultCode   free(Context* context);

    // Enumerate the adapter information for the given api, returns the number of adapters enumerated.
    virtual u32         enumerateAdapterInformation(Adapter::Information* adapterInformation, u32 maxAdapters) = 0;

    // Create an adapter for the given index, returns nullptr if the adapter doesn't exist.
    virtual Adapter*    createAdapter(uint index) = 0;

    // Free the given adapter.
    virtual ResultCode  freeAdapter(Adapter* adapter) = 0;

    Api                 getApi() const { return m_api; }
    uint                getId() const { return m_id; }

private:
    Api                 m_api;
    uint                m_id;
};

typedef Context* ContextHandle;
} // RenderApi
} // Recluse
#endif // RECLUSE_RENDER_API_INSTANCE_HPP