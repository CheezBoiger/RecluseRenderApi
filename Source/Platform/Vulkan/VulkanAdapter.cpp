//
#include "VulkanAdapter.hpp"
#include "VulkanDevice.hpp"
#include "SupportGraph.hpp"

#include <Recluse/Messaging.hpp>

#include <map>

namespace Recluse {
namespace RenderApi {
namespace Vulkan {

static uint kAdapterCounter = 0;

std::map<VulkanAdapter*, std::map<VkDevice, VulkanDevice>> g_deviceMap;

VkPhysicalDeviceProperties VulkanAdapter::gatherProperties(VkPhysicalDevice physicalDevice)
{
    VkPhysicalDeviceProperties properties = { };
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);
    return properties;
}

Adapter::Information VulkanAdapter::gatherInformation(const VkPhysicalDeviceProperties& properties, uint index)
{
    Adapter::Information adapterInformation = { };
    adapterInformation.index = index;
    adapterInformation.vendorId = properties.vendorID;
    adapterInformation.deviceId = properties.deviceID;

    strncpy(adapterInformation.name, properties.deviceName, 128);

    switch (properties.deviceType)
    {
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            adapterInformation.type = Adapter::Type_CPU;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            adapterInformation.type = Adapter::Type_Discrete;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            adapterInformation.type = Adapter::Type_Integrated;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            adapterInformation.type = Adapter::Type_Virtual;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_OTHER:
        default:
            adapterInformation.type = Adapter::Type_Unknown;
            break;
    }

    return adapterInformation;
}

VulkanAdapter::VulkanAdapter(VulkanContext* context, VkPhysicalDevice physDevice, uint adapterIndex)
    : Adapter(++kAdapterCounter)
    , m_physicalDevice(physDevice)
    , m_context(context)
    , m_adapterIndex(adapterIndex)
{
    initialize();
}

ResultCode VulkanAdapter::queryInformation(Information* info)
{
    *info = gatherInformation(gatherProperties(m_physicalDevice), m_adapterIndex);
    return RecluseResult_Ok;
}

Bool VulkanAdapter::supportsFeature(FeatureFlag feature)
{
    return false;
}

Device* VulkanAdapter::createDevice(const Device::Description& description)
{
    std::vector<const char*> requestedExtensions;

    {
        std::vector<std::string> initialExt;

        SupportGraph graph;

        graph(VK_KHR_16BIT_STORAGE_EXTENSION_NAME, SupportGraph::Extension::Device)
            (VK_KHR_8BIT_STORAGE_EXTENSION_NAME, SupportGraph::Extension::Device)
            (VK_KHR_RAY_QUERY_EXTENSION_NAME, SupportGraph::Extension::Device)
            (VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, SupportGraph::Extension::Device)
            (VK_EXT_MESH_SHADER_EXTENSION_NAME, SupportGraph::Extension::Device)
#if defined(VK_NV_mesh_shader)
            (VK_NV_MESH_SHADER_EXTENSION_NAME, SupportGraph::Extension::Device)
#endif
            (VK_KHR_SPIRV_1_4_EXTENSION_NAME, SupportGraph::Extension::Device)
            (VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME, SupportGraph::Extension::Device)
            (VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME, SupportGraph::Extension::Device)
            (VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, SupportGraph::Extension::Device)
            (VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME, SupportGraph::Extension::Device)
            (VK_KHR_MULTIVIEW_EXTENSION_NAME, SupportGraph::Extension::Device)
            (VK_KHR_MAINTENANCE2_EXTENSION_NAME, SupportGraph::Extension::Device)
            (VK_NV_SHADER_IMAGE_FOOTPRINT_EXTENSION_NAME, SupportGraph::Extension::Device)
            (VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME, SupportGraph::Extension::Device)
            (VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME, SupportGraph::Extension::Device)
            (VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME, SupportGraph::Extension::Device)
            (VK_KHR_DEVICE_GROUP_EXTENSION_NAME, SupportGraph::Extension::Device)
            (VK_KHR_MAINTENANCE3_EXTENSION_NAME, SupportGraph::Extension::Device)
            (VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME, SupportGraph::Extension::Device)
            (VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME, SupportGraph::Extension::Device)
            (VK_KHR_MAINTENANCE1_EXTENSION_NAME, SupportGraph::Extension::Device)
            (VK_KHR_STORAGE_BUFFER_STORAGE_CLASS_EXTENSION_NAME, SupportGraph::Extension::Device)
            (VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, SupportGraph::Extension::Instance);

        // Graph link dependencies
        graph
            (VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME, { 
                { VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, true } })

            (VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, { 
                { VK_KHR_SPIRV_1_4_EXTENSION_NAME, true },
                { VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, true } })

            (VK_KHR_RAY_QUERY_EXTENSION_NAME, { 
                { VK_KHR_SPIRV_1_4_EXTENSION_NAME, true },
                { VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, true } })

            (VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, { 
                { VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME, true },
                { VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME, true },
                { VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME, true } })

            (VK_EXT_MESH_SHADER_EXTENSION_NAME, { 
                { VK_KHR_SPIRV_1_4_EXTENSION_NAME, true } })

            (VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME, { 
                { VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, true },
                { VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME, true } })

            (VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME, { 
                { VK_KHR_MULTIVIEW_EXTENSION_NAME, true },
                { VK_KHR_MAINTENANCE2_EXTENSION_NAME, true } })

            (VK_KHR_MULTIVIEW_EXTENSION_NAME, { 
                { VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, true } })

            (VK_NV_SHADER_IMAGE_FOOTPRINT_EXTENSION_NAME, { 
                { VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, true } })

            (VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME, { 
                { VK_KHR_MAINTENANCE3_EXTENSION_NAME, true },
                { VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, true } })

            (VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME, { 
                { VK_KHR_DEVICE_GROUP_EXTENSION_NAME, true },
                { VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, true } })

            (VK_KHR_MAINTENANCE3_EXTENSION_NAME, { 
                { VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, true } })
    
            (VK_KHR_SPIRV_1_4_EXTENSION_NAME, { 
                { VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME, true } })

            (VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME, { 
                { VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, true } })

            (VK_KHR_16BIT_STORAGE_EXTENSION_NAME, { 
                { VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, true },
                { VK_KHR_STORAGE_BUFFER_STORAGE_CLASS_EXTENSION_NAME, true } })

            (VK_KHR_16BIT_STORAGE_EXTENSION_NAME, { 
                { VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, true },
                { VK_KHR_STORAGE_BUFFER_STORAGE_CLASS_EXTENSION_NAME, true } });

        initialExt.push_back(VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME);
        initialExt.push_back(VK_KHR_MAINTENANCE1_EXTENSION_NAME);

        if (description.enableMeshShaders)
        {
            initialExt.push_back(VK_EXT_MESH_SHADER_EXTENSION_NAME);
        }

        if (description.enableRayTracing)
        {
            initialExt.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
            initialExt.push_back(VK_KHR_RAY_QUERY_EXTENSION_NAME);
            initialExt.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
        }

        if (description.enableVariableRateShading)
        {
            initialExt.push_back(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME);
        }

        if (description.enableSamplerFeedback)
        {
            initialExt.push_back(VK_NV_SHADER_IMAGE_FOOTPRINT_EXTENSION_NAME);
        }

        requestedExtensions = graph.queryAllExtensions(initialExt);
    }

    uint32_t queueFamilyPropertyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyPropertyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyPropertyCount, queueFamilyProperties.data());
    // TODO: Query queues to be made.

    VkDeviceCreateInfo deviceCi = { };
    deviceCi.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCi.enabledLayerCount = 0;
    deviceCi.ppEnabledLayerNames = nullptr;

    return nullptr;
}

ResultCode VulkanAdapter::freeDevice(Device* device)
{
    return RecluseResult_Ok;
}

VkPhysicalDeviceMemoryProperties VulkanAdapter::getMemoryProperties() const
{
    return m_memoryProperties.memoryProperties;
}

VkPhysicalDeviceMemoryBudgetPropertiesEXT VulkanAdapter::getBudgetProperties() const
{
    return m_memoryBudgets;
}

void VulkanAdapter::initialize()
{
    R_ASSERT(m_physicalDevice != VK_NULL_HANDLE);

    m_memoryBudgets = { };
    m_memoryBudgets.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT;

    m_memoryProperties = { };
    m_memoryProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
    m_memoryProperties.pNext = &m_memoryBudgets;

    vkGetPhysicalDeviceMemoryProperties2(m_physicalDevice, &m_memoryProperties);
    
}
} // Vulkan
} // RenderApi
} // Recluse