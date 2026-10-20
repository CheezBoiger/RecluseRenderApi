//
#include <Recluse/RenderApi/Resource.hpp>
#include <Recluse/Serialization/Hasher.hpp>
#include <unordered_map>

namespace Recluse {
namespace RenderApi {
namespace Database {
static std::unordered_map<Hash64, ResourceState> g_resourceStates;


ResourceState obtainResourceState(ResourceId id)
{
    auto it = g_resourceStates.find(recluseHashFast(&id, sizeof(ResourceId)));
    if (it != g_resourceStates.end()) {
        return it->second;
    }
    return ResourceState_Unknown;
}


void setResourceState(ResourceId id, ResourceState state)
{
    const Hash64 hash = recluseHashFast(&id, sizeof(ResourceId));
    auto it = g_resourceStates.find(hash);
    if (it != g_resourceStates.end()) {
        it->second = state;
    }
    else 
    {
        g_resourceStates[hash] = state;
    }
}
} // Database
} // RenderApi
} // Recluse