#ifndef RECLUSE_VULKAN_ALLOCATOR_HPP
#define RECLUSE_VULKAN_ALLOCATOR_HPP

#include "VulkanCommon.hpp"
#include <Recluse/Memory/Allocator.hpp>
#include <Recluse/Memory/BuddyAllocationStrategy.hpp>

#include <vector>
#include <array>

namespace Recluse {
namespace RenderApi {
namespace Vulkan {

enum AllocationHeapType
{
    AllocationHeapType_DeviceLocal,
    AllocationHeapType_HostWrite,
};

struct AllocationInstance
{
    VkDeviceMemory      memory;
    VkDeviceSize        totalHeapSizeBytes;
    U32                 bytesAllocated;
    VkMemoryHeapFlags   flags;
    AllocationHeapType  heapType;
    U32                 heapIndex;
};

struct Allocation
{
    VkDeviceMemory      memory;
    U32                 offsetBytes;
    U32                 sizeBytes;
    U32                 allocatorIndex;
    AllocationHeapType  allocationType;
};

struct AllocationFreeRequest
{
    Allocation  allocation;
    U32         index;      // The buffer index that requested this allocation be freed.
                            // Index helps identifies if the allocation is being used by a frame in flight, which means 
                            // we would need to wait or avoid cleanup while it is active.
};

class HeapInstance
{
public:
    HeapInstance() 
        : m_totalMemory(0ull)
        , m_memoryTypeIndex(0ull) 
        , m_memoryHandle(VK_NULL_HANDLE)
    { }

    ~HeapInstance() { }

    VkResult        allocateMemory(VkMemoryRequirements& requirements, VkMemoryPropertyFlags properties, VkDeviceMemory& memory);
    void            freeMemory(VkDeviceMemory memory);

    void            initialize(VkDevice device, VkDeviceSize totalHeapSize, U32 memoryTypeIndex);
    void            release(VkDevice device);

    B32             isInitialized() const { return m_memoryHandle != VK_NULL_HANDLE; }
    
private:
    VkDeviceMemory              m_memoryHandle;
    VkDeviceSize                m_totalMemory;
    uint32_t                    m_memoryTypeIndex;
    Allocator<BuddyStrategy>    m_allocator;
};

class AllocatorHandler
{
public:
    void        initialize(U32 bufferCount);
    void        release();

    Allocation  allocate(VkDeviceSize requestedSizeBytes, ResourceMemoryUsage usage);
    void        cleanUpResources(U32 bufferIndex);

    void        incrementAndUpdate();

private:
    struct AllocationContext
    {
        std::array<std::vector<HeapInstance>, ResourceMemoryUsage_Count> instances;
        
    };
    
    std::vector<AllocationContext> m_allocationContext;
};
} // Vulkan
} // RenderApi
} // Recluse
#endif // RECLUSE_VULKAN_ALLOCATOR_HPP