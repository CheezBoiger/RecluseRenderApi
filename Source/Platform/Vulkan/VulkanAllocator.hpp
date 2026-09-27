#ifndef RECLUSE_VULKAN_ALLOCATOR_HPP
#define RECLUSE_VULKAN_ALLOCATOR_HPP

#include "VulkanCommon.hpp"
#include <Recluse/Memory/Allocator.hpp>
#include <Recluse/Memory/BuddyAllocationStrategy.hpp>

#include <vector>

namespace Recluse {
namespace RenderApi {
namespace Vulkan {

class VulkanAllocatorInstance
{
public:
    VulkanAllocatorInstance(VkDevice device = VK_NULL_HANDLE, VkDeviceSize memorySizeBytes, uint32_t memoryTypeIndex) 
        : m_device(device)
        , m_totalMemory(memorySizeBytes)
        , m_memoryTypeIndex(memoryTypeIndex) 
    { initialize(); }

    ~VulkanAllocatorInstance() { release(); }

    VkResult    allocateMemory(VkMemoryRequirements& requirements, VkMemoryPropertyFlags properties, VkDeviceMemory& memory);
    void        freeMemory(VkDeviceMemory memory);

private:

    void initialize();
    void release();

    VkDevice m_device;
    VkDeviceSize m_totalMemory;
    uint32_t m_memoryTypeIndex;
};
} // Vulkan
} // RenderApi
} // Recluse
#endif // RECLUSE_VULKAN_ALLOCATOR_HPP