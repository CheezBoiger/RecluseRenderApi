//
#include <Recluse/RenderApi/Context.hpp>
#include <Recluse/RenderApi/Device.hpp>
#include <Recluse/System/DLLLoader.hpp>
#include <Recluse/Messaging.hpp>

#include "Shared/CommandOps.hpp"
#include "ResourceState.hpp"

#include <map>
#include <vector>
#include <set>

namespace Recluse {
namespace RenderApi {

typedef Context* (*CreateContextFn)(const Context::Description& description);
typedef ResultCode(*FreeContextFn)(Context* context);

struct LibraryContext
{
    DllLoader           loader;
    CreateContextFn     createContextFn;
    FreeContextFn       freeContextFn;
    std::set<Context*>  contexts;

    LibraryContext()
        : createContextFn(nullptr)
        , freeContextFn(nullptr) { }
};

std::map<Api, LibraryContext> g_libraryContexts;

Context* Context::create(Api api, const Description& description)
{
    Context* context = nullptr;
    auto it = g_libraryContexts.find(api);
    if (it == g_libraryContexts.end())
    {
        LibraryContext libContext;
        switch (api)
        {
        case Api::Direct3D11:
            libContext.loader.load("RecluseRenderApiD3D11.dll");
            break;
        case Api::Direct3D12:
            libContext.loader.load("RecluseRenderApiD3D12.dll");
            break;
        case Api::Vulkan:
            libContext.loader.load("RecluseVulkan.dll");
            break;
        case Api::OpenGL:
            libContext.loader.load("RecluseRenderApiOpenGL.dll");
            break;
        case Api::SoftwareRaster:
            libContext.loader.load("RecluseRenderApiSoftwareRaster.dll");
            break;
        case Api::SoftwareRaytrace:
            libContext.loader.load("RecluseRenderApiSoftwareRaytrace.dll");
            break;
        default:
            return nullptr;
        }
        if (libContext.loader.isLoaded())
        {
            libContext.createContextFn = (CreateContextFn)libContext.loader.procAddress("createContext");
            libContext.freeContextFn = (FreeContextFn)libContext.loader.procAddress("freeContext");

            g_libraryContexts[api] = std::move(libContext);
            it = g_libraryContexts.find(api);
        }
        else
        {
            R_ERROR("DeviceCreator", "Failed to load render api library for api: %d", (int)api);
            return nullptr;
        }
    }

    // We need to make sure that the library context is valid and has the proper function pointers.
    if (it->second.createContextFn && it->second.freeContextFn)
    {
        context = it->second.createContextFn(description);
        if (context)
        {
            // Insert the context into the library context.
            it->second.contexts.insert(context);
        }
    }

    return context;
}


ResultCode Context::free(Context* context)
{
    if (!context) return RecluseResult_NullPtrExcept;
    Api api = context->getApi();
    auto it = g_libraryContexts.find(api);
    if (it != g_libraryContexts.end())
    {
        auto& libContext = it->second;
        if (libContext.freeContextFn)
        {
            ResultCode result = libContext.freeContextFn(context);
            if (result == RecluseResult_Ok)
            {
                libContext.contexts.erase(context);
            }

            // Unload if there are no more contexts for this library context.
            if (libContext.contexts.empty())
            {
                libContext.loader.unload();
                g_libraryContexts.erase(it);
            }

            return result;
        }
    }

    return RecluseResult_NotFound;
}

ResultCode FrameProcess::submitCommandLists(CommandQueueType type, CommandList** lists, uint numCommandLists)
{
    for (uint i = 0; i < numCommandLists; ++i)
    {
        if (!lists[i]) return RecluseResult_NullPtrExcept;
        // Processing initial transitions for initial resources.
        const std::vector<UPtr>& initialTransitions = lists[i]->m_initialTransitions;
        for (uint j = 0; j < initialTransitions.size(); ++j)
        {
            Transition* transition = (Transition*)initialTransitions[j];
            if (!transition) continue;
            Resource* resource = transition->resource;
            if (!resource) continue;
            ResourceState globalCurrentState = Database::obtainResourceState(resource->getId());
            auto it = lists[i]->m_localResourceStateMap.find(resource->getId().value);
            if (it != lists[i]->m_localResourceStateMap.end())
            {
                transition->oldState = globalCurrentState;
            }
        }

        // Update the global resource state for all resources in the command list.
        const std::unordered_map<uint, LocalResourceState>& localResourceStateMap = lists[i]->m_localResourceStateMap;
        for (auto& it : localResourceStateMap)
        {
            ResourceId resourceId = { it.first };
            Database::setResourceState(resourceId, it.second.currentState);
        }
    }

    return onSubmitCommandLists(type, lists, numCommandLists);
}
} // RenderApi
} // Recluse