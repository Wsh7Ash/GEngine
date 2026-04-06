#pragma once

// ================================================================
//  AssetDependencyGraph.h
//  Manages asset dependencies and change propagation.
// ================================================================

#include "AssetMetadata.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <algorithm>

namespace ge {
namespace assets {

class AssetDependencyGraph {
public:
    AssetDependencyGraph() = default;
    
    void AddAsset(const AssetMetadata& metadata);
    void RemoveAsset(AssetHandle handle);
    
    void AddDependency(AssetHandle dependent, AssetHandle dependency);
    void RemoveDependency(AssetHandle dependent, AssetHandle dependency);
    
    std::vector<AssetHandle> GetDependencies(AssetHandle handle) const;
    std::vector<AssetHandle> GetDependents(AssetHandle handle) const;
    
    std::vector<AssetHandle> GetAllDependentsRecursive(AssetHandle handle) const;
    
    bool HasDependency(AssetHandle dependent, AssetHandle dependency) const;
    bool HasCircularDependency(AssetHandle a, AssetHandle b) const;
    
    std::vector<AssetHandle> TopologicalSort() const;
    
    void Clear();
    size_t GetAssetCount() const { return assets_.size(); }
    
    bool Contains(AssetHandle handle) const { 
        return assets_.find(handle) != assets_.end(); 
    }

private:
    std::unordered_map<AssetHandle, AssetMetadata> assets_;
    
    std::unordered_map<AssetHandle, std::vector<AssetHandle>> dependencies_;
    std::unordered_map<AssetHandle, std::vector<AssetHandle>> dependents_;
    
    void EnsureEntry(AssetHandle handle);
    void PropagateDependent(AssetHandle dependency, AssetHandle newDependent);
    void RemoveDependentFrom(AssetHandle dependency, AssetHandle removedDependent);
};

inline void AssetDependencyGraph::AddAsset(const AssetMetadata& metadata) {
    assets_[metadata.Handle] = metadata;
    EnsureEntry(metadata.Handle);
}

inline void AssetDependencyGraph::RemoveAsset(AssetHandle handle) {
    auto depIt = dependencies_.find(handle);
    if (depIt != dependencies_.end()) {
        for (auto& dep : depIt->second) {
            auto& dependentsList = dependents_[dep];
            dependentsList.erase(
                std::remove(dependentsList.begin(), dependentsList.end(), handle),
                dependentsList.end()
            );
        }
        dependencies_.erase(depIt);
    }
    
    auto relIt = dependents_.find(handle);
    if (relIt != dependents_.end()) {
        for (auto& rel : relIt->second) {
            auto& depsList = dependencies_[rel];
            depsList.erase(
                std::remove(depsList.begin(), depsList.end(), handle),
                depsList.end()
            );
        }
        dependents_.erase(relIt);
    }
    
    assets_.erase(handle);
}

inline void AssetDependencyGraph::AddDependency(AssetHandle dependent, AssetHandle dependency) {
    if (dependent == dependency || !Contains(dependency)) return;
    
    EnsureEntry(dependent);
    EnsureEntry(dependency);
    
    auto& deps = dependencies_[dependent];
    if (std::find(deps.begin(), deps.end(), dependency) == deps.end()) {
        deps.push_back(dependency);
        dependents_[dependency].push_back(dependent);
        
        if (assets_.find(dependent) != assets_.end()) {
            assets_[dependent].AddDependency(dependency);
        }
        if (assets_.find(dependency) != assets_.end()) {
            assets_[dependency].AddDependent(dependent);
        }
    }
}

inline void AssetDependencyGraph::RemoveDependency(AssetHandle dependent, AssetHandle dependency) {
    auto depIt = dependencies_.find(dependent);
    if (depIt == dependencies_.end()) return;
    
    depIt->second.erase(
        std::remove(depIt->second.begin(), depIt->second.end(), dependency),
        depIt->second.end()
    );
    
    auto& revDependents = dependents_[dependency];
    revDependents.erase(
        std::remove(revDependents.begin(), revDependents.end(), dependent),
        revDependents.end()
    );
    
    if (assets_.find(dependent) != assets_.end()) {
        assets_[dependent].RemoveDependency(dependency);
    }
    if (assets_.find(dependency) != assets_.end()) {
        assets_[dependency].RemoveDependent(dependent);
    }
}

inline std::vector<AssetHandle> AssetDependencyGraph::GetDependencies(AssetHandle handle) const {
    auto it = dependencies_.find(handle);
    if (it != dependencies_.end()) {
        return it->second;
    }
    return {};
}

inline std::vector<AssetHandle> AssetDependencyGraph::GetDependents(AssetHandle handle) const {
    auto it = dependents_.find(handle);
    if (it != dependents_.end()) {
        return it->second;
    }
    return {};
}

inline std::vector<AssetHandle> AssetDependencyGraph::GetAllDependentsRecursive(AssetHandle handle) const {
    std::vector<AssetHandle> result;
    std::unordered_set<AssetHandle> visited;
    
    std::function<void(AssetHandle)> traverse;
    traverse = [&](AssetHandle h) {
        if (visited.count(h)) return;
        visited.insert(h);
        
        auto it = dependents_.find(h);
        if (it != dependents_.end()) {
            for (auto dependent : it->second) {
                result.push_back(dependent);
                traverse(dependent);
            }
        }
    };
    
    traverse(handle);
    return result;
}

inline bool AssetDependencyGraph::HasDependency(AssetHandle dependent, AssetHandle dependency) const {
    auto it = dependencies_.find(dependent);
    if (it == dependencies_.end()) return false;
    return std::find(it->second.begin(), it->second.end(), dependency) != it->second.end();
}

inline bool AssetDependencyGraph::HasCircularDependency(AssetHandle a, AssetHandle b) const {
    std::unordered_set<AssetHandle> visited;
    std::function<bool(AssetHandle)> dfs;
    dfs = [&](AssetHandle current) -> bool {
        if (current == a) return true;
        if (visited.count(current)) return false;
        visited.insert(current);
        
        auto it = dependencies_.find(current);
        if (it != dependencies_.end()) {
            for (auto dep : it->second) {
                if (dfs(dep)) return true;
            }
        }
        return false;
    };
    
    return dfs(b);
}

inline std::vector<AssetHandle> AssetDependencyGraph::TopologicalSort() const {
    std::unordered_map<AssetHandle, int> inDegree;
    std::unordered_map<AssetHandle, std::vector<AssetHandle>> adj;
    
    for (const auto& [handle, deps] : dependencies_) {
        if (inDegree.find(handle) == inDegree.end()) {
            inDegree[handle] = 0;
        }
        for (auto dep : deps) {
            adj[dep].push_back(handle);
            inDegree[handle]++;
        }
    }
    
    std::queue<AssetHandle> q;
    for (const auto& [handle, degree] : inDegree) {
        if (degree == 0) q.push(handle);
    }
    
    std::vector<AssetHandle> result;
    while (!q.empty()) {
        auto current = q.front();
        q.pop();
        result.push_back(current);
        
        for (auto neighbor : adj[current]) {
            inDegree[neighbor]--;
            if (inDegree[neighbor] == 0) {
                q.push(neighbor);
            }
        }
    }
    
    return result;
}

inline void AssetDependencyGraph::Clear() {
    assets_.clear();
    dependencies_.clear();
    dependents_.clear();
}

inline void AssetDependencyGraph::EnsureEntry(AssetHandle handle) {
    if (dependencies_.find(handle) == dependencies_.end()) {
        dependencies_[handle] = {};
    }
    if (dependents_.find(handle) == dependents_.end()) {
        dependents_[handle] = {};
    }
}

inline void AssetDependencyGraph::PropagateDependent(AssetHandle dependency, AssetHandle newDependent) {
    auto it = dependents_.find(dependency);
    if (it != dependents_.end()) {
        if (std::find(it->second.begin(), it->second.end(), newDependent) == it->second.end()) {
            it->second.push_back(newDependent);
        }
    }
    
    for (auto dep : GetDependencies(dependency)) {
        PropagateDependent(dep, newDependent);
    }
}

inline void AssetDependencyGraph::RemoveDependentFrom(AssetHandle dependency, AssetHandle removedDependent) {
    auto it = dependents_.find(dependency);
    if (it != dependents_.end()) {
        it->second.erase(
            std::remove(it->second.begin(), it->second.end(), removedDependent),
            it->second.end()
        );
    }
    
    for (auto dep : GetDependencies(dependency)) {
        RemoveDependentFrom(dep, removedDependent);
    }
}

} // namespace assets
} // namespace ge