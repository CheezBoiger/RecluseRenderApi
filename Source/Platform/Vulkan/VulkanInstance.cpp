//
#include "VulkanInstance.hpp"
#include "VulkanContext.hpp"

#include <Recluse/Messaging.hpp>

#include <map>

std::map<Recluse::uint, Recluse::RenderApi::Vulkan::VulkanContext*> g_contextMap;

Recluse::RenderApi::Context* createContext(const Recluse::RenderApi::Context::Description& description)
{
    Recluse::RenderApi::Vulkan::VulkanContext* ctx = new Recluse::RenderApi::Vulkan::VulkanContext(description);
    auto it = g_contextMap.find(ctx->getId());
    R_ASSERT(it == g_contextMap.end());
    
    if (it == g_contextMap.end())
    {
        g_contextMap.insert(std::make_pair(ctx->getId(), ctx));
    }
    else
    {
        delete ctx;
        ctx = nullptr;
    }

    return ctx;
}

Recluse::ResultCode freeContext(Recluse::RenderApi::Context* context)
{
    if (!context) return Recluse::RecluseResult_NullPtrExcept;

    auto it = g_contextMap.find(context->getId());
    if (it != g_contextMap.end())
    {
        delete it->second;
        g_contextMap.erase(it);
        return Recluse::RecluseResult_Ok;
    }
    return Recluse::RecluseResult_NotFound;
}