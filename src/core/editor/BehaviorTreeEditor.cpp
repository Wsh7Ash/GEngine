#include "BehaviorTreeEditor.h"
#include <imgui.h>
#include <imgui_internal.h>

namespace ge {
namespace editor {

void BehaviorTreeEditor::OnImGuiRender(BehaviorTreeComponent* btComponent) {
    if (!currentTree_ && btComponent) {
        currentTree_ = &btComponent->tree;
    }
    
    if (ImGui::Begin("Behavior Tree Editor", nullptr, ImGuiWindowFlags_MenuBar)) {
        if (ImGui::BeginMenuBar()) {
            RenderToolbar();
            ImGui::EndMenuBar();
        }
        
        ImVec2 contentSize = ImGui::GetContentRegionAvail();
        
        if (ImGui::BeginChild("NodePalette", ImVec2(150, 0), true)) {
            RenderNodePalette();
            ImGui::EndChild();
            
            ImGui::SameLine();
        }
        
        if (ImGui::BeginChild("Canvas", ImVec2(0, 0), true)) {
            ImVec2 canvasPos = ImGui::GetCursorScreenPos();
            ImVec2 canvasSize = ImGui::GetContentRegionAvail();
            
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            ImU32 bgColor = IM_COL32(40, 40, 45, 255);
            drawList->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), bgColor);
            
            if (currentTree_ && !currentTree_->nodes.empty()) {
                float startX = 50.0f;
                float startY = 50.0f;
                float spacing = 100.0f;
                
                for (const auto& node : currentTree_->nodes) {
                    if (node.id == currentTree_->rootNodeId) {
                        RenderNode(node, startX, startY, spacing);
                        break;
                    }
                }
            }
            
            ImGui::EndChild();
        }
        
        if (selectedNodeId_ != 0 && currentTree_) {
            BehaviorTreeNode* node = currentTree_->FindNode(selectedNodeId_);
            if (node) {
                ImGui::SameLine();
                if (ImGui::BeginChild("Properties", ImVec2(200, 0), true)) {
                    RenderNodeProperties(*node);
                    ImGui::EndChild();
                }
            }
        }
    }
    ImGui::End();
}

void BehaviorTreeEditor::RenderNode(const BehaviorTreeNode& node, float& x, float y, float spacing) {
    auto it = nodePositions_.find(node.id);
    ImVec2 nodePos;
    if (it != nodePositions_.end()) {
        nodePos = it->second;
    } else {
        nodePos = ImVec2(x, y);
        nodePositions_[node.id] = nodePos;
    }
    
    ImVec2 nodeSize = ImVec2(120, 50);
    ImVec2 nodeMin = nodePos;
    ImVec2 nodeMax = ImVec2(nodePos.x + nodeSize.x, nodePos.y + nodeSize.y);
    
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    
    ImU32 bgColor = IM_COL32(60, 60, 70, 255);
    if (selectedNodeId_ == node.id) {
        bgColor = IM_COL32(80, 120, 180, 255);
    }
    
    ImU32 borderColor = IM_COL32(100, 100, 120, 255);
    drawList->AddRectFilled(nodeMin, nodeMax, bgColor, 4.0f);
    drawList->AddRect(nodeMin, nodeMax, borderColor, 4.0f);
    
    ImGui::SetCursorScreenPos(ImVec2(nodePos.x + 10, nodePos.y + 15));
    ImGui::Text("%s", node.name.c_str());
    
    if (ImGui::IsMouseClicked(0) && ImGui::IsMouseHoveringRect(nodeMin, nodeMax)) {
        selectedNodeId_ = node.id;
    }
    
    if (!node.children.empty()) {
        float childX = x + spacing;
        float childY = y;
        
        for (size_t i = 0; i < node.children.size(); ++i) {
            BehaviorTreeNode* childNode = currentTree_->FindNode(node.children[i]);
            if (childNode) {
                float lineStartX = nodeMax.x;
                float lineStartY = nodePos.y + nodeSize.y / 2;
                
                float lineEndX = childX - 10;
                float lineEndY = childY + 25;
                
                drawList->AddLine(
                    ImVec2(lineStartX, lineStartY),
                    ImVec2(lineEndX, lineEndY),
                    IM_COL32(150, 150, 180, 255), 2.0f
                );
                
                RenderNode(*childNode, childX, childY, spacing);
                childY += 80;
            }
        }
    }
}

void BehaviorTreeEditor::RenderNodeProperties(BehaviorTreeNode& node) {
    ImGui::Text("Node Properties");
    ImGui::Separator();
    
    char nameBuffer[128];
    strncpy_s(nameBuffer, node.name.c_str(), sizeof(nameBuffer) - 1);
    if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer))) {
        node.name = nameBuffer;
    }
    
    ImGui::Text("Type: %s", node.type == BehaviorNodeType::Action ? "Action" :
                           node.type == BehaviorNodeType::Condition ? "Condition" :
                           node.type == BehaviorNodeType::Sequence ? "Sequence" :
                           node.type == BehaviorNodeType::Selector ? "Selector" :
                           node.type == BehaviorNodeType::Decorator ? "Decorator" : "Unknown");
    
    if (node.type == BehaviorNodeType::Decorator) {
        const char* decoratorNames[] = { "None", "Inverter", "Repeater", "UntilFail", "UntilSuccess", "Cooldown", "Timeout" };
        int currentDecorator = static_cast<int>(node.decorator);
        if (ImGui::Combo("Decorator", &currentDecorator, decoratorNames, 7)) {
            node.decorator = static_cast<DecoratorType>(currentDecorator);
        }
    }
    
    ImGui::Separator();
    ImGui::Text("Parameters");
    
    for (auto& param : node.parameters) {
        char keyBuffer[64];
        char valueBuffer[128];
        strncpy_s(keyBuffer, param.first.c_str(), sizeof(keyBuffer) - 1);
        strncpy_s(valueBuffer, param.second.c_str(), sizeof(valueBuffer) - 1);
        
        if (ImGui::InputText(("Key: " + param.first).c_str(), keyBuffer, sizeof(keyBuffer))) {
            param.first = keyBuffer;
        }
        if (ImGui::InputText(("Value: " + param.second).c_str(), valueBuffer, sizeof(valueBuffer))) {
            param.second = valueBuffer;
        }
    }
    
    if (ImGui::Button("Add Parameter")) {
        node.parameters.push_back({"param", "value"});
    }
    
    ImGui::Separator();
    
    if (ImGui::Button("Add Child")) {
        BehaviorTreeNode newNode(BehaviorNodeType::Action, "New Action");
        newNode.id = static_cast<uint64_t>(currentTree_->nodes.size() + 1);
        currentTree_->AddNode(newNode);
        node.children.push_back(newNode.id);
    }
    
    if (ImGui::Button("Delete Node", ImVec2(-1, 0))) {
        if (node.id != currentTree_->rootNodeId) {
            currentTree_->RemoveNode(node.id);
            selectedNodeId_ = 0;
        }
    }
}

void BehaviorTreeEditor::RenderToolbar() {
    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("New Tree")) {
            currentTree_ = nullptr;
            nodePositions_.clear();
            selectedNodeId_ = 0;
        }
        if (ImGui::MenuItem("Save")) {
            
        }
        if (ImGui::MenuItem("Load")) {
            
        }
        ImGui::EndMenu();
    }
    
    if (ImGui::BeginMenu("Edit")) {
        if (ImGui::MenuItem("Add Node")) {
            if (currentTree_) {
                BehaviorTreeNode newNode(BehaviorNodeType::Action, "New Action");
                newNode.id = static_cast<uint64_t>(currentTree_->nodes.size() + 1);
                currentTree_->AddNode(newNode);
            }
        }
        ImGui::EndMenu();
    }
}

void BehaviorTreeEditor::RenderNodePalette() {
    ImGui::Text("Node Types");
    ImGui::Separator();
    
    if (ImGui::Button("Sequence", ImVec2(-1, 0))) {
        if (currentTree_) {
            BehaviorTreeNode node(BehaviorNodeType::Sequence, "Sequence");
            node.id = static_cast<uint64_t>(currentTree_->nodes.size() + 1);
            currentTree_->AddNode(node);
            if (currentTree_->rootNodeId == 0) {
                currentTree_->rootNodeId = node.id;
            }
        }
    }
    
    if (ImGui::Button("Selector", ImVec2(-1, 0))) {
        if (currentTree_) {
            BehaviorTreeNode node(BehaviorNodeType::Selector, "Selector");
            node.id = static_cast<uint64_t>(currentTree_->nodes.size() + 1);
            currentTree_->AddNode(node);
            if (currentTree_->rootNodeId == 0) {
                currentTree_->rootNodeId = node.id;
            }
        }
    }
    
    if (ImGui::Button("Action", ImVec2(-1, 0))) {
        if (currentTree_) {
            BehaviorTreeNode node(BehaviorNodeType::Action, "Action");
            node.id = static_cast<uint64_t>(currentTree_->nodes.size() + 1);
            currentTree_->AddNode(node);
        }
    }
    
    if (ImGui::Button("Condition", ImVec2(-1, 0))) {
        if (currentTree_) {
            BehaviorTreeNode node(BehaviorNodeType::Condition, "Condition");
            node.id = static_cast<uint64_t>(currentTree_->nodes.size() + 1);
            currentTree_->AddNode(node);
        }
    }
    
    if (ImGui::Button("Decorator", ImVec2(-1, 0))) {
        if (currentTree_) {
            BehaviorTreeNode node(BehaviorNodeType::Decorator, "Decorator");
            node.id = static_cast<uint64_t>(currentTree_->nodes.size() + 1);
            currentTree_->AddNode(node);
        }
    }
}

}
}
