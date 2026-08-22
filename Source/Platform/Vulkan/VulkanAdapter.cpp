//
#include "VulkanAdapter.hpp"
#include "VulkanDevice.hpp"
#include "SupportGraph.hpp"

#include <Recluse/Messaging.hpp>

#include <map>
#include <functional>

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

Bool VulkanAdapter::checkSupportsDeviceExtension(VkPhysicalDevice physicalDevice, const char* ext)
{
    std::vector<VkExtensionProperties> deviceExtensions = gatherExtensionProperties(physicalDevice);
    for (U32 i = 0; i < deviceExtensions.size(); ++i)
    {
        // We found the right extension
        if (strcmp(ext, deviceExtensions[i].extensionName) == 0)
        {
            return true;
        }
    }
    // No supported extension found, return false.
    return false;
}

static VulkanDevice::QueueIndices obtainQueueIndicesData(VkQueueFamilyProperties* properties, uint propertyCount,
    CommandQueueType* types, uint typesCount)
{
    VulkanDevice::QueueIndices indices = { };
    typedef std::function<Bool(VkQueueFlags)> ConditionCheckFunc;
    std::vector<uint> queueCounter(propertyCount); 

    auto queryFunc = [&] (VulkanDevice::QueueProperties& queueProps, Bool separateQueue, ConditionCheckFunc check) -> void {
        for (uint i = 0; i < propertyCount; ++i)
        {
            VkQueueFamilyProperties& prop = properties[i];
            // In addition to checking conditions for the right queue, we also need to check if 
            // there are enough queues in the queue family to satisify our needs.
            if (check(prop.queueFlags))
            {
                queueProps.familyIndex = i;

                if ((separateQueue || queueCounter[i] == 0) && (queueCounter[i] < prop.queueCount))
                    queueCounter[i]++;

                queueProps.queueIndex = queueCounter[i] - 1;
                break;
            }
        }
    };

    for (uint i = 0; i < typesCount; ++i)
    {
        if (types[i] == CommandQueueType_Graphics)
            queryFunc(indices.graphics, true, [] (VkQueueFlags flags) -> Bool { return flags & VK_QUEUE_GRAPHICS_BIT; });

        if (types[i] == CommandQueueType_Compute)
        {
            queryFunc(indices.compute, true, [] (VkQueueFlags flags) -> Bool { return (flags & VK_QUEUE_COMPUTE_BIT) && !(flags & VK_QUEUE_GRAPHICS_BIT); });
            if (indices.compute.familyIndex == VulkanDevice::QueueProperties::kBadIndex)
                queryFunc(indices.compute, true, [] (VkQueueFlags flags) -> Bool { return flags & VK_QUEUE_COMPUTE_BIT; });
        }
       
        if (types[i] == CommandQueueType_Copy)
        {
            queryFunc(indices.copy, true, [] (VkQueueFlags flags) -> Bool { return (flags & VK_QUEUE_TRANSFER_BIT) && !(flags & VK_QUEUE_GRAPHICS_BIT) && !(flags && VK_QUEUE_COMPUTE_BIT); });
            if (indices.copy.familyIndex == VulkanDevice::QueueProperties::kBadIndex)
                queryFunc(indices.copy, true, [] (VkQueueFlags flags) -> Bool { return (flags & VK_QUEUE_TRANSFER_BIT) && !(flags && VK_QUEUE_COMPUTE_BIT); });
            // If still nothing, try finally for anything with transfer.
            if (indices.copy.familyIndex == VulkanDevice::QueueProperties::kBadIndex)
                queryFunc(indices.copy, true, [] (VkQueueFlags flags) -> Bool { return flags & VK_QUEUE_TRANSFER_BIT; });
        }
    }

    // Default refill if any of these types are not explicitly requested by the user.
    if (indices.graphics.familyIndex == VulkanDevice::QueueProperties::kBadIndex)
        queryFunc(indices.graphics, false, [] (VkQueueFlags flags) -> Bool { return flags & VK_QUEUE_GRAPHICS_BIT; });

    if (indices.compute.familyIndex == VulkanDevice::QueueProperties::kBadIndex)
        queryFunc(indices.compute, false, [] (VkQueueFlags flags) -> Bool { return flags & VK_QUEUE_COMPUTE_BIT; });

    if (indices.copy.familyIndex == VulkanDevice::QueueProperties::kBadIndex)
        queryFunc(indices.copy, false, [] (VkQueueFlags flags) -> Bool { return flags & VK_QUEUE_TRANSFER_BIT; });

    return indices;
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

    switch (properties.vendorID) 
    {
        case AMD_VENDOR_ID: 
            adapterInformation.vendorName = "Advanced Micro Devices"; 
            break;
        case INTEL_VENDOR_ID:  
            adapterInformation.vendorName = "Intel Corporation"; 
            break;
        case NVIDIA_VENDOR_ID:  
            adapterInformation.vendorName = "Nvidia Corporation"; 
            break;
        case MSFT_VENDOR_ID: 
            adapterInformation.vendorName = "Microsoft"; 
            break;
        case QUALCOMM_VENDOR_ID: 
            adapterInformation.vendorName = "Qualcomm Technologies"; 
            break;
        default:
            adapterInformation.vendorName = "Unknown"; 
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

std::vector<VkExtensionProperties> VulkanAdapter::gatherExtensionProperties(VkPhysicalDevice physicalDevice)
{
    uint32_t propertyCount = 0;
    VkResult result = vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &propertyCount, nullptr);
    R_ASSERT(result == VK_SUCCESS);
    std::vector<VkExtensionProperties> extensionProperties(propertyCount);
    result = vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &propertyCount, extensionProperties.data());
    R_ASSERT(result == VK_SUCCESS);
    return extensionProperties;
}

static void checkAvailableExtensions(VkPhysicalDevice physicalDevice, std::vector<const char*>& requestedExtensions)
{
    std::vector<VkExtensionProperties> extensionProperties = VulkanAdapter::gatherExtensionProperties(physicalDevice);
    for (int i = 0; i < requestedExtensions.size(); ++i)
    {
        Bool found = false;
        for (const auto& it : extensionProperties)
        {
            if (strcmp(it.extensionName, requestedExtensions[i]) == 0)
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            R_ERROR("Vulkan" "Extension %s is not available for this physical device!", requestedExtensions[i]);
            requestedExtensions.erase(requestedExtensions.begin() + i);
            --i;
        }
    }
}

Device* VulkanAdapter::createDevice(const Device::Description& description)
{
    SupportGraph graph;
    std::vector<const char*> requestedExtensions;

    {
        std::vector<std::string> initialExt;

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
            (VK_KHR_SWAPCHAIN_EXTENSION_NAME, SupportGraph::Extension::Device)
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
        initialExt.push_back(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME);
        initialExt.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

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

        requestedExtensions = graph.queryAllExtensions(initialExt, SupportGraph::Extension::Device);
    }

    checkAvailableExtensions(m_physicalDevice, requestedExtensions);
    

    uint32_t queueFamilyPropertyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyPropertyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyPropertyCount, queueFamilyProperties.data());

    VulkanDevice::QueueIndices indices = obtainQueueIndicesData(queueFamilyProperties.data(), queueFamilyPropertyCount, description.queueTypes, description.queueTypeCount);
    std::vector<VkDeviceQueueCreateInfo> deviceQueueCi;
    std::vector<std::vector<float>> queuePriorities;

    auto createDeviceQueueCi = [&] (const VulkanDevice::QueueProperties& queueProps) -> void {
        if (queueProps.familyIndex != VulkanDevice::QueueProperties::kBadIndex)
        {
            Bool found = false;
            for (uint i = 0; i < deviceQueueCi.size(); ++i)
            {
                if (deviceQueueCi[i].queueFamilyIndex == queueProps.familyIndex)
                {
                    found = true;
                    queuePriorities[i].push_back(1.0f);

                    deviceQueueCi[i].queueCount = Math::maximum(deviceQueueCi[i].queueCount, queueProps.queueIndex + 1);
                    // Re-update the priorities as it is invalid from the addition.
                    deviceQueueCi[i].pQueuePriorities = queuePriorities[i].data();

                    break;
                }
            }

            if (!found)
            {
                queuePriorities.push_back(std::vector<float>{ 1.0f });

                VkDeviceQueueCreateInfo queueCi = { };
                queueCi.sType               = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueCi.queueCount          = 1;
                queueCi.queueFamilyIndex    = queueProps.familyIndex;
                queueCi.pQueuePriorities    = queuePriorities.back().data();
                queueCi.flags               = 0;
        
                deviceQueueCi.push_back(queueCi);
            }
        }
    };

    createDeviceQueueCi(indices.graphics);
    createDeviceQueueCi(indices.compute);
    createDeviceQueueCi(indices.copy);

    Features features;

    if (description.enableMeshShaders)
    {
        VkPhysicalDeviceMeshShaderFeaturesEXT* meshShader = features.add<VkPhysicalDeviceMeshShaderFeaturesEXT>();
        meshShader->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
    }

    vkGetPhysicalDeviceFeatures2(m_physicalDevice, &features());

    if (description.enableMeshShaders)
    {
        VkPhysicalDeviceMeshShaderFeaturesEXT* meshShader = features.find<VkPhysicalDeviceMeshShaderFeaturesEXT>();
        if (meshShader)
        {
            meshShader->multiviewMeshShader = false;
            meshShader->primitiveFragmentShadingRateMeshShader = false;
        }
    }

    VkDeviceCreateInfo deviceCi         = { };
    deviceCi.sType                      = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCi.enabledLayerCount          = 0;
    deviceCi.ppEnabledLayerNames        = nullptr;
    deviceCi.enabledExtensionCount      = requestedExtensions.size();
    deviceCi.ppEnabledExtensionNames    = requestedExtensions.data();
    deviceCi.queueCreateInfoCount       = deviceQueueCi.size();
    deviceCi.pQueueCreateInfos          = deviceQueueCi.data();
    deviceCi.pEnabledFeatures           = nullptr; // To be using VkPhysicalDeviceFeatures2
    deviceCi.pNext                      = &features();
    
    VkDevice device = VK_NULL_HANDLE;
    VkResult result = vkCreateDevice(m_physicalDevice, &deviceCi, nullptr, &device);
    R_ASSERT(result == VK_SUCCESS);
    
    if (result == VK_SUCCESS)
    {
        g_deviceMap[this].insert(std::make_pair(device, VulkanDevice(this, device, indices)));
        return &g_deviceMap[this][device];
    }
    
    return nullptr;
}

ResultCode VulkanAdapter::freeDevice(Device* device)
{
    if (!device) return RecluseResult_NullPtrExcept;

    VulkanDevice* native = dynamic_cast<VulkanDevice*>(device);
    if (native)
    {
        auto it = g_deviceMap[this].find(native->get());
        if (it == g_deviceMap[this].end())
        {
            return RecluseResult_NotFound;
        }
    
        native->release();
        g_deviceMap[this].erase(it);
    }
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