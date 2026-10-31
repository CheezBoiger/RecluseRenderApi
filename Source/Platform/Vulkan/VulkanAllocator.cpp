//
#include "VulkanAllocator.hpp"

namespace Recluse {
namespace RenderApi {
namespace Vulkan {


void HeapInstance::initialize(VkDevice device, VkDeviceSize totalHeapSize, U32 memoryTypeIndex)
{
    VkMemoryAllocateInfo allocateInfo   = { };
    allocateInfo.sType                  = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.memoryTypeIndex        = memoryTypeIndex;
    allocateInfo.allocationSize         = totalHeapSize;

    VkResult result = vkAllocateMemory(device, &allocateInfo, nullptr, &m_memoryHandle);
    R_ASSERT_FORMAT(result == VK_SUCCESS, "Failed to allocate memory heap.");

    if (result == VK_SUCCESS)
    {
        // Memory allocator is initialized to 0, as the memory address is the heap base + offset.
        m_allocator.initialize((UPtr)0ull, totalHeapSize);

        // Needs to be mapped if the resource is not gpumemory.
    }
}

void HeapInstance::release(VkDevice device)
{
    if (m_memoryHandle != VK_NULL_HANDLE)
    {
        vkFreeMemory(device, m_memoryHandle, nullptr);
    }

    m_memoryHandle = VK_NULL_HANDLE;
}


} // Vulkan
} // RenderApi
} // Recluse