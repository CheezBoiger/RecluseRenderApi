#ifndef RECLUSE_SUPPORT_GRAPH_HPP
#define RECLUSE_SUPPORT_GRAPH_HPP

#pragma once

#include <Recluse/Types.hpp>
#include <Recluse/Serialization/Hasher.hpp>

#include <vector>
#include <map>
#include <set>

namespace Recluse {

// SupportGraph maps extensions and strings to their dependencies, 
// which are used for vulkan device extension querying.
class SupportGraph
{
public:
    struct Extension
    {
        enum Type { Device, Instance };
        std::string extension;
        Type type;
    };

    struct DependencyInput
    {
        std::string extension;
        Bool required;
    };

    struct Dependency
    {
        Hash64 hash;
        Bool required;

        bool operator<(const Dependency& o) const 
        {
            return hash < o.hash;
        }
    };

    struct Dependencies
    {
        std::set<Dependency> dependencies;
    };

    Bool                            addExtension(const std::string& extension, Extension::Type type);
    Bool                            linkDependencies(const std::string& extension, const std::vector<DependencyInput>& dependencies);
    std::vector<DependencyInput>    getDependencies(const std::string& extension, Extension::Type type);

    std::vector<const char*>        queryAllExtensions(const std::vector<std::string>& requestedExtensions, Extension::Type type);

    SupportGraph& operator()(const std::string& extension, Extension::Type type)
    {
        addExtension(extension, type);
        return *this;
    }

    SupportGraph& operator()(const std::string& extension, const std::vector<DependencyInput>& dependents)
    {
        linkDependencies(extension, dependents);
        return *this;
    }

private:
    std::map<Hash64, Dependencies> dependencyGraph;
    std::map<Hash64, Extension> extensionMap;
};
} // Recluse
#endif // RECLUSE_SUPPORT_GRAPH_HPP