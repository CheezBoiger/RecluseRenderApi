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

std::vector<SupportGraph::DependencyInput> SupportGraph::getDependencies(const std::string& extension, Extension::Type type)
{
    std::vector<DependencyInput> results;
    Hash64 extHash = recluseHashFast(extension.data(), extension.size());
    auto it = dependencyGraph.find(extHash);
    if (it != dependencyGraph.end())
    {
        Dependencies& dependencies = it->second;
        for (auto depIt = dependencies.dependencies.begin(); depIt != dependencies.dependencies.end(); ++depIt)
        {
            auto depExtIt = extensionMap.find(depIt->hash);
            // Add if the type also applies.
            if (depExtIt != extensionMap.end() && (depExtIt->second.type == type))
            {
                DependencyInput input { depExtIt->second.extension, depIt->required };
                results.push_back(input);
            }
        }
    }
    return results;
}

std::vector<const char*> SupportGraph::queryAllExtensions(const std::vector<std::string>& requestedExtensions)
{
    std::vector<const char*> extensions;
    return extensions;
}
} // Recluse