//
#include <Recluse/RenderApi/Context.hpp>

#include <Recluse/System/DLLLoader.hpp>
#include <Recluse/Messaging.hpp>

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
} // RenderApi
} // Recluse