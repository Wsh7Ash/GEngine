#pragma once

// ================================================================
//  SystemGraph.h
//  Analyzes system dependencies and builds execution frames.
// ================================================================

#include "System.h"
#include "SystemManager.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>

namespace ge {
namespace ecs {

struct SystemNode {
    std::type_index type;
    std::string name;
    System* system = nullptr;
    Signature readSignature;
    Signature writeSignature;
    SystemPriority priority = SystemPriority::Normal;
    bool isParallelizable = true;
    
    int frame = -1;
    int orderInFrame = 0;
};

struct ExecutionFrame {
    std::vector<SystemNode*> systems;
    float estimatedTime = 0.0f;
    bool canRunParallel = true;
};

class SystemGraph {
public:
    SystemGraph() = default;
    ~SystemGraph() = default;
    
    void Build(SystemManager* systemManager);
    
    const std::vector<ExecutionFrame>& GetFrames() const { return frames_; }
    std::vector<ExecutionFrame>& GetFrames() { return frames_; }
    
    const std::vector<SystemNode>& GetNodes() const { return nodes_; }
    std::vector<SystemNode>& GetNodes() { return nodes_; }
    
    bool HasConflicts() const { return hasConflicts_; }
    
    void PrintExecutionPlan() const;
    
    bool AreSystemsParallelizable(const SystemNode& a, const SystemNode& b) const;
    
private:
    void CollectSystems(SystemManager* systemManager);
    void AnalyzeDependencies();
    void BuildFrames();
    void SortByPriority();
    
    bool HasWriteConflict(const SystemNode& a, const SystemNode& b) const;
    bool HasReadWriteConflict(const SystemNode& a, const SystemNode& b) const;
    
    std::vector<SystemNode> nodes_;
    std::vector<ExecutionFrame> frames_;
    bool hasConflicts_ = false;
    
    std::unordered_map<std::type_index, SystemNode*> nodeMap_;
};

} // namespace ecs
} // namespace ge
