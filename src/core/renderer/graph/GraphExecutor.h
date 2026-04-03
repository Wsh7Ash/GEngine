#pragma once

// ================================================================
//  GraphExecutor.h
//  Execution engine with automatic barrier insertion.
// ================================================================

#include "RenderGraph.h"
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <cstdint>

namespace ge {
namespace renderer {

enum class BarrierType {
    None,
    ColorAttachment,
    DepthStencilAttachment,
    ShaderResource,
    UnorderedAccess,
    Resolve,
    Generic
};

struct PipelineBarrier {
    std::string textureName;
    TextureState oldState;
    TextureState newState;
    BarrierType type;
    uint32_t subresource = 0xFFFFFFFF;
};

struct PassBarrierInfo {
    std::vector<PipelineBarrier> preExecute;
    std::vector<PipelineBarrier> postExecute;
};

class GraphExecutor {
public:
    GraphExecutor(RenderGraph& graph);
    ~GraphExecutor();
    
    void Initialize();
    void Shutdown();
    
    void Execute();
    void Execute(uint32_t width, uint32_t height);
    
    void AddBarrier(const std::string& textureName, TextureState from, TextureState to);
    void AddBarriers(const std::vector<PipelineBarrier>& barriers);
    
    void ClearBarriers();
    
    const PassBarrierInfo& GetBarrierInfo(const std::string& passName) const;
    PassBarrierInfo& GetBarrierInfo(const std::string& passName);
    
    void SetAutoBarriersEnabled(bool enabled) { autoBarriersEnabled_ = enabled; }
    bool IsAutoBarriersEnabled() const { return autoBarriersEnabled_; }
    
    std::function<void(const PipelineBarrier&)> onBarrierInserted;
    
private:
    void CalculateBarriers();
    void InsertBarrier(const PipelineBarrier& barrier);
    TextureState DetermineRequiredState(const std::string& passName, const std::string& textureName);
    void ExecuteBarriers(const std::vector<PipelineBarrier>& barriers);
    
    RenderGraph& graph_;
    
    std::unordered_map<std::string, PassBarrierInfo> passBarrierInfo_;
    std::vector<PipelineBarrier> pendingBarriers_;
    
    std::unordered_map<std::string, TextureState> currentStates_;
    
    bool isInitialized_ = false;
    bool autoBarriersEnabled_ = true;
};

class BarrierScheduler {
public:
    BarrierScheduler() = default;
    ~BarrierScheduler() = default;
    
    std::vector<PipelineBarrier> ScheduleBarriers(
        const std::vector<std::string>& passOrder,
        const std::unordered_map<std::string, std::vector<std::string>>& passInputs,
        const std::unordered_map<std::string, std::vector<std::string>>& passOutputs,
        const std::unordered_map<std::string, TextureState>& initialStates);
    
    void SetTransitionCost(float cost) { transitionCost_ = cost; }
    float GetTransitionCost() const { return transitionCost_; }
    
private:
    struct ResourceTransition {
        std::string resourceName;
        TextureState fromState;
        TextureState toState;
        int passIndex;
    };
    
    float transitionCost_ = 0.1f;
    
    bool IsStateTransitionRequired(TextureState from, TextureState to) const;
    BarrierType DetermineBarrierType(TextureState from, TextureState to) const;
};

} // namespace renderer
} // namespace ge
