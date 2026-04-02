#include "ecs/SystemGraph.h"
#include "debug/log.h"
#include <algorithm>

namespace ge {
namespace ecs {

void SystemGraph::Build(SystemManager* systemManager) {
    nodes_.clear();
    frames_.clear();
    
    CollectSystems(systemManager);
    AnalyzeDependencies();
    BuildFrames();
    SortByPriority();
    
    GE_CORE_INFO("SystemGraph: Built {} systems in {} frames", nodes_.size(), frames_.size());
}

void SystemGraph::CollectSystems(SystemManager* systemManager) {
    for (const auto& pair : systemManager->systems_) {
        SystemNode node;
        node.type = pair.first;
        node.system = pair.second.get();
        node.readSignature = pair.second->GetReadSignature();
        node.writeSignature = pair.second->GetWriteSignature();
        node.priority = pair.second->GetPriority();
        node.isParallelizable = pair.second->IsParallelizable();
        
        nodes_.push_back(node);
        nodeMap_[pair.first] = &nodes_.back();
    }
}

void SystemGraph::AnalyzeDependencies() {
    hasConflicts_ = false;
    
    for (size_t i = 0; i < nodes_.size(); ++i) {
        for (size_t j = i + 1; j < nodes_.size(); ++j) {
            if (!AreSystemsParallelizable(nodes_[i], nodes_[j])) {
                hasConflicts_ = true;
            }
        }
    }
}

bool SystemGraph::AreSystemsParallelizable(const SystemNode& a, const SystemNode& b) const {
    if (!a.isParallelizable || !b.isParallelizable) {
        return false;
    }
    
    return !HasWriteConflict(a, b) && !HasReadWriteConflict(a, b);
}

bool SystemGraph::HasWriteConflict(const SystemNode& a, const SystemNode& b) const {
    for (size_t i = 0; i < MAX_COMPONENTS; ++i) {
        if (a.writeSignature[i] && b.writeSignature[i]) {
            return true;
        }
    }
    return false;
}

bool SystemGraph::HasReadWriteConflict(const SystemNode& a, const SystemNode& b) const {
    Signature aWritesBReads = a.writeSignature & b.readSignature;
    Signature bWritesAReads = b.writeSignature & a.readSignature;
    
    for (size_t i = 0; i < MAX_COMPONENTS; ++i) {
        if (aWritesBReads[i] || bWritesAReads[i]) {
            return true;
        }
    }
    return false;
}

void SystemGraph::BuildFrames() {
    std::vector<bool> assigned(nodes_.size(), false);
    int currentFrame = 0;
    
    while (true) {
        ExecutionFrame frame;
        
        for (size_t i = 0; i < nodes_.size(); ++i) {
            if (assigned[i]) continue;
            
            bool canAdd = true;
            for (auto* sys : frame.systems) {
                if (!AreSystemsParallelizable(nodes_[i], *sys)) {
                    canAdd = false;
                    break;
                }
            }
            
            if (canAdd) {
                nodes_[i].frame = currentFrame;
                nodes_[i].orderInFrame = frame.systems.size();
                frame.systems.push_back(&nodes_[i]);
                frame.estimatedTime += nodes_[i].system->GetEstimatedTime();
                assigned[i] = true;
            }
        }
        
        if (frame.systems.empty()) {
            for (size_t i = 0; i < nodes_.size(); ++i) {
                if (!assigned[i]) {
                    nodes_[i].frame = currentFrame;
                    frame.systems.push_back(&nodes_[i]);
                    assigned[i] = true;
                    frame.canRunParallel = false;
                    break;
                }
            }
        }
        
        if (frame.systems.empty()) break;
        
        frames_.push_back(std::move(frame));
        ++currentFrame;
    }
}

void SystemGraph::SortByPriority() {
    for (auto& frame : frames_) {
        std::sort(frame.systems.begin(), frame.systems.end(),
            [](SystemNode* a, SystemNode* b) {
                if (a->priority != b->priority) {
                    return a->priority < b->priority;
                }
                return a->orderInFrame < b->orderInFrame;
            });
    }
}

void SystemGraph::PrintExecutionPlan() const {
    GE_CORE_INFO("=== System Execution Plan ===");
    
    for (size_t i = 0; i < frames_.size(); ++i) {
        const auto& frame = frames_[i];
        GE_CORE_INFO("Frame {} ({} systems, {}ms estimated):", 
            i, frame.systems.size(), frame.estimatedTime);
        
        for (auto* sys : frame.systems) {
            GE_CORE_INFO("  - {} (priority: {}, parallelizable: {})",
                sys->name, (int)sys->priority, sys->isParallelizable);
        }
    }
}

} // namespace ecs
} // namespace ge
