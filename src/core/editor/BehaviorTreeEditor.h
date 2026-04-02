#pragma once

#include "../behaviortree/BehaviorTreeNode.h"
#include "../behaviortree/BehaviorTreeComponent.h"
#include <imgui.h>
#include <string>
#include <vector>
#include <unordered_map>

namespace ge {

namespace editor {

class BehaviorTreeEditor {
public:
    BehaviorTreeEditor() = default;
    ~BehaviorTreeEditor() = default;
    
    void OnImGuiRender(BehaviorTreeComponent* btComponent);
    
    void SetBehaviorTree(BehaviorTree* tree) { currentTree_ = tree; }
    BehaviorTree* GetBehaviorTree() { return currentTree_; }
    
    void SetSelectedNode(uint64_t nodeId) { selectedNodeId_ = nodeId; }
    uint64_t GetSelectedNode() const { return selectedNodeId_; }
    
private:
    void RenderNode(const BehaviorTreeNode& node, float& x, float y, float spacing);
    void RenderNodeProperties(BehaviorTreeNode& node);
    void RenderToolbar();
    void RenderNodePalette();
    
    BehaviorTree* currentTree_ = nullptr;
    uint64_t selectedNodeId_ = 0;
    float scrollX_ = 0.0f;
    float scrollY_ = 0.0f;
    bool isDragging_ = false;
    uint64_t draggingNodeId_ = 0;
    ImVec2 dragOffset_ = {0, 0};
    
    std::unordered_map<uint64_t, ImVec2> nodePositions_;
    std::unordered_map<std::string, BehaviorNodeType> nodePalette_;
};

}
}
