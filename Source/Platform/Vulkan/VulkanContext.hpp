#ifndef RECLUSE_VULKAN_CONTEXT_HPP
#define RECLUSE_VULKAN_CONTEXT_HPP
#pragma once

#include <Recluse/RenderApi/Context.hpp>
#include "VulkanCommon.hpp"

#include <RecluseRenderApi_exports.hpp>
#include <map>

namespace Recluse {
namespace RenderApi {
namespace Vulkan {

class VulkanAdapter;

class RecluseRenderApi_PUBLIC_API VulkanContext : public Context
{
public:
    VulkanContext(const Description& description, uint apiVersion = R_VULKAN_DEFAULT_VERSION());
    ~VulkanContext();

    void            initialize(const Description& description);
    ResultCode      destroy();

    u32             enumerateAdapterInformation(Adapter::Information* adapterInformation, u32 maxAdapters) override;

    Adapter*        createAdapter(uint adapter) override;
    ResultCode      freeAdapter(Adapter* adapter) override;

    Bool            isValid() const override;

    uint32_t        getApiVersion() const { return m_vulkanApiVersion; }
    uint32_t        getApiVersionMajor() const { return VK_VERSION_MAJOR(m_vulkanApiVersion); }
    uint32_t        getApiVersionMinor() const { return VK_VERSION_MINOR(m_vulkanApiVersion); }
    uint32_t        getApiVersionPatch() const { return VK_VERSION_PATCH(m_vulkanApiVersion); }

    VkSurfaceKHR    makeSurface(WindowHandle handle);

private:

    void            queryCallbacks();
    void            registerValidationCallback();
    void            unregisterValidationCallback();
    void            cleanUpSurfaces();

    // Instance for vulkan.
    VkInstance                          m_instance;
    uint32_t                            m_totalPhysicalDevices;
    uint32_t                            m_vulkanApiVersion;

    VkDebugUtilsMessengerEXT            m_debugMessenger;
    PFN_vkCreateDebugUtilsMessengerEXT  pfnCreateDebugUtilsMessengerEXT;
    PFN_vkDestroyDebugUtilsMessengerEXT pfnDestroyDebugUtilsMessengerEXT;

    std::map<WindowHandle, VkSurfaceKHR> m_surfaceMap;
};
} // Vulkan
} // RenderApi
} // Recluse
#endif // RECLUSE_VULKAN_CONTEXT_HPP