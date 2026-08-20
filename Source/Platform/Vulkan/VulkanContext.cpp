//
#include <Recluse/Messaging.hpp>
#include <Recluse/Math/MathCommons.hpp>
#include <Recluse/Serialization/Hasher.hpp>

#include "SupportGraph.hpp"
#include "VulkanContext.hpp"
#include "VulkanAdapter.hpp"

#include <vector>

namespace Recluse {
namespace RenderApi {
namespace Vulkan {

static std::map<VulkanContext*, std::map<VkPhysicalDevice, VulkanAdapter>> m_adapterMap;
static uint kContextCounter = 0;

static std::vector<const char*> enumerateAndCheckLayers(FeatureFlags flags)
{
    std::vector<const char*> layerOptions;

    // Load the layer options.
    if (flags & FeatureFlag_Validation)
        layerOptions.push_back("VK_LAYER_KHRONOS_validation");

    if (flags & FeatureFlag_ApiDump)
        layerOptions.push_back("VK_LAYER_LUNARG_api_dump");

    uint32_t layerCount = 0;
    VkResult result = vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    R_ASSERT(result == VK_SUCCESS);
    std::vector<VkLayerProperties> properties(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, properties.data());
    R_ASSERT(result == VK_SUCCESS);

    for (uint i = 0; i < layerOptions.size(); ++i)
    {
        const char* layer = layerOptions[i];
        Bool found = false;

        for (uint j = 0; j < layerCount; ++j)
        {
            VkLayerProperties& props = properties[j];
            if (strcmp(props.layerName, layer) == 0)
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            // If not found, remove
            layerOptions.erase(layerOptions.begin() + i);
            --i;
        }
    }

    return layerOptions;
}

static std::vector<const char*> enumerateAndCheckExtensions(FeatureFlags flags)
{
    std::vector<const char*> extensionOptions {
        VK_KHR_SURFACE_EXTENSION_NAME,
#if defined(RECLUSE_WINDOWS)
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
    };

    std::vector<VkExtensionProperties> properties;
    uint32_t propertyCount = 0;
    VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &propertyCount, nullptr);
    R_ASSERT(result == VK_SUCCESS);
    properties.resize(propertyCount);
    result = vkEnumerateInstanceExtensionProperties(nullptr, &propertyCount, properties.data());
    R_ASSERT(result == VK_SUCCESS);

    for (uint i = 0; i < extensionOptions.size(); ++i)
    {
        const char* ext = extensionOptions[i];
        Bool found = false;
        for (uint j = 0;  j < propertyCount; ++j)
        {
            VkExtensionProperties& property = properties[j];
            if (strcmp(property.extensionName, ext) == 0)
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            extensionOptions.erase(extensionOptions.begin() + i);
            --i;
        }
    }
    R_ASSERT(result == VK_SUCCESS);
    

    return extensionOptions;
}

VulkanContext::VulkanContext(const Description& description)
    : Context(Api::Vulkan, ++kContextCounter)
    , m_instance(VK_NULL_HANDLE)
    , m_totalPhysicalDevices(0)
{
    initialize(description);
}

void VulkanContext::initialize(const Description& description)
{
    VkApplicationInfo applicationInfo       = { };
    VkInstanceCreateInfo instanceCreateInfo = { };

    std::vector<const char*> layers = enumerateAndCheckLayers(description.flags);
    std::vector<const char*> extensions = enumerateAndCheckExtensions(description.flags);

    applicationInfo.sType               = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.pEngineName         = description.engineName;
    applicationInfo.pApplicationName    = description.applicationName;
    applicationInfo.apiVersion          = VK_MAKE_API_VERSION(0, 1, 4, 0);
    applicationInfo.applicationVersion  = VK_MAKE_VERSION(0, 1, 0);
    applicationInfo.engineVersion       = VK_MAKE_VERSION(0, 1, 0);
    
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &applicationInfo;
    instanceCreateInfo.enabledLayerCount = layers.size();
    instanceCreateInfo.ppEnabledLayerNames = layers.data();
    instanceCreateInfo.enabledExtensionCount = extensions.size();
    instanceCreateInfo.ppEnabledExtensionNames = extensions.data();
    instanceCreateInfo.flags = 0;
    
    VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance);
    if (result != VK_SUCCESS)
    {
        R_ERROR("Vulkan", "Failed to created instance, error code=%d", result);
        m_instance = nullptr;
    }
    else
    {
        // Query physical devices ahead of time.
        VkResult result = vkEnumeratePhysicalDevices(m_instance, &m_totalPhysicalDevices, nullptr);
        R_ASSERT(result == VK_SUCCESS);
    }
}

VulkanContext::~VulkanContext()
{
    destroy();
}

u32 VulkanContext::enumerateAdapterInformation(Adapter::Information* adapterInformation, u32 maxAdapters)
{
    uint32_t deviceCount = m_totalPhysicalDevices;

    if (deviceCount == 0) // We shouldn't need to re-query again...
    {
        VkResult result = vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
        m_totalPhysicalDevices = deviceCount;
        R_ASSERT(result == VK_SUCCESS);
    }

    if (maxAdapters != 0)
    {
        deviceCount = Math::minimum(m_totalPhysicalDevices, maxAdapters);

        std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
        VkResult result = vkEnumeratePhysicalDevices(m_instance, &deviceCount, physicalDevices.data());

        R_ASSERT(result == VK_SUCCESS);

        for (uint i = 0; i < maxAdapters; ++i)
        {
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(physicalDevices[i], &properties);
            adapterInformation[i].index = i;
            adapterInformation[i].vendorId = properties.vendorID;
            adapterInformation[i].deviceId = properties.deviceID;

            strncpy(adapterInformation[i].name, properties.deviceName, 128);

            switch (properties.deviceType)
            {
                case VK_PHYSICAL_DEVICE_TYPE_CPU:
                    adapterInformation[i].type = Adapter::Type_CPU;
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                    adapterInformation[i].type = Adapter::Type_Discrete;
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                    adapterInformation[i].type = Adapter::Type_Integrated;
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                    adapterInformation[i].type = Adapter::Type_Virtual;
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                default:
                    adapterInformation[i].type = Adapter::Type_Unknown;
                    break;
            }
        }
    }
    
    return deviceCount;
}

Adapter* VulkanContext::createAdapter(uint adapter)
{
    R_ASSERT(m_instance != VK_NULL_HANDLE);

    Adapter* nativeAdapter = nullptr;
    if (m_totalPhysicalDevices != 0)
    {
        std::vector<VkPhysicalDevice> devices{m_totalPhysicalDevices};
        VkResult result = vkEnumeratePhysicalDevices(m_instance, &m_totalPhysicalDevices, devices.data());
        R_ASSERT(adapter < m_totalPhysicalDevices);
        R_ASSERT(result == VK_SUCCESS);

        // Early out if we asked for an index that overlaps the total number of physical devices..
        if (adapter >= m_totalPhysicalDevices)
            return nullptr;

        VkPhysicalDevice device = devices[adapter];
        auto it = m_adapterMap[this].find(device);
        if (it == m_adapterMap[this].end())
        {
            m_adapterMap[this].insert(std::make_pair(devices[adapter], VulkanAdapter(devices[adapter])));
            it = m_adapterMap[this].find(device);
        }
        nativeAdapter = &it->second;        
    }
    return nativeAdapter;
}

ResultCode VulkanContext::freeAdapter(Adapter* adapter)
{
    if (!adapter) return RecluseResult_NullPtrExcept;
    VulkanAdapter* vulkanAdapter = dynamic_cast<VulkanAdapter*>(adapter);
    if (vulkanAdapter)
    {
        auto it = m_adapterMap[this].find(vulkanAdapter->get());
        if (it != m_adapterMap[this].end())
        {
            m_adapterMap[this].erase(it);
            return RecluseResult_Ok;
        }
    }
    return RecluseResult_NotFound;
}

Bool VulkanContext::isValid() const
{
    return (m_instance != VK_NULL_HANDLE);
}

ResultCode VulkanContext::destroy()
{
    if (m_instance)
    {
        vkDestroyInstance(m_instance, nullptr);
    }

    m_instance = VK_NULL_HANDLE;
    m_totalPhysicalDevices = 0;
    return RecluseResult_Ok;
}
} // Vulkan
} // RenderApi
} // Recluse