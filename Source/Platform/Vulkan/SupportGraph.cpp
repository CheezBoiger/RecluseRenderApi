//
#include "SupportGraph.hpp"

namespace Recluse {

Bool SupportGraph::addExtension(const std::string& extension, Extension::Type type)
{
    Hash64 h = recluseHashFast(extension.data(), extension.size());
    auto it = extensionMap.find(h);
    if (it == extensionMap.end())
    {
        extensionMap.insert(std::make_pair(h, Extension{extension, type}));
        dependencyGraph.insert(std::make_pair(h, Dependencies()));
        return true;
    }
    return false;
}

Bool SupportGraph::linkDependencies(const std::string& extension, const std::vector<DependencyInput>& dependencies)
{
    Hash64 h = recluseHashFast(extension.data(), extension.size());
    auto it = extensionMap.find(h);
    if (it != extensionMap.end())
    {
        Dependencies& node = dependencyGraph[h];
        for (const auto& d : dependencies)
        {
            Hash64 depH = recluseHashFast(d.extension.data(), d.extension.size());
            auto dep = extensionMap.find(depH);
            if (dep == extensionMap.end())
            {
                return false;
            }
            node.dependencies.insert({depH, d.required});
        }
        return true;
    }

    return false;
}

std::vector<const char*> SupportGraph::queryAllExtensions(const std::vector<std::string>& requestedExtensions)
{
    std::vector<const char*> extensions;
    return extensions;
}
} // Recluse