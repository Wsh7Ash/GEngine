#include "VisualScriptEditor.h"
#include "../debug/log.h"
#include "imgui.h"
#include <algorithm>
#include <sstream>

namespace ge {
namespace editor {

visualscripting::VisualGraph VisualScriptEditor::currentGraph_;
std::vector<EditorNode> VisualScriptEditor::editorNodes_;
std::vector<EditorConnection> VisualScriptEditor::editorConnections_;
std::string VisualScriptEditor::selectedNodeId_;
bool VisualScriptEditor::showNodePalette_ = true;
bool VisualScriptEditor::showPropertyPanel_ = true;
float VisualScriptEditor::canvasOffsetX_ = 0;
float VisualScriptEditor::canvasOffsetY_ = 0;
float VisualScriptEditor::canvasZoom_ = 1.0f;
std::string VisualScriptEditor::currentFilePath_;
std::string VisualScriptEditor::statusMessage_;
bool VisualScriptEditor::isDraggingConnection_ = false;
std::string VisualScriptEditor::dragFromNodeId_;
std::string VisualScriptEditor::dragFromPinId_;
float VisualScriptEditor::dragStartX_ = 0;
float VisualScriptEditor::dragStartY_ = 0;
float VisualScriptEditor::dragEndX_ = 0;
float VisualScriptEditor::dragEndY_ = 0;
int VisualScriptEditor::nextNodeId_ = 1;

void VisualScriptEditor::Init() {
    visualscripting::NodeTypeRegistry::RegisterBuiltInNodes();
    GE_LOG_INFO("VisualScriptEditor: Initialized");
}

void VisualScriptEditor::Shutdown() {
    editorNodes_.clear();
    editorConnections_.clear();
    GE_LOG_INFO("VisualScriptEditor: Shutdown");
}

void VisualScriptEditor::Render() {
    RenderMenuBar();

    ImGui::Begin("Visual Script Editor");

    ImVec2 availSize = ImGui::GetContentRegionAvail();
    float paletteWidth = 200;
    float propertyWidth = 250;

    if (showNodePalette_) {
        ImGui::BeginChild("NodePalette", ImVec2(paletteWidth, availSize.y), true);
        RenderNodePalette();
        ImGui::EndChild();
        ImGui::SameLine();
    }

    float canvasWidth = availSize.x - (showNodePalette_ ? paletteWidth : 0) - (showPropertyPanel_ ? propertyWidth : 0);
    ImGui::BeginChild("Canvas", ImVec2(canvasWidth, availSize.y), true);
    RenderGraphCanvas();
    ImGui::EndChild();

    if (showPropertyPanel_) {
        ImGui::SameLine();
        ImGui::BeginChild("Properties", ImVec2(propertyWidth, availSize.y), true);
        RenderPropertyPanel();
        ImGui::EndChild();
    }

    if (!statusMessage_.empty()) {
        ImGui::TextUnformatted(statusMessage_.c_str());
    }

    ImGui::End();
}

void VisualScriptEditor::RenderMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Visual Script")) {
            if (ImGui::MenuItem("New Graph")) {
                NewGraph("NewGraph");
            }
            if (ImGui::MenuItem("Save", "Ctrl+S")) {
                if (!currentFilePath_.empty()) {
                    SaveGraph(currentFilePath_);
                }
            }
            if (ImGui::MenuItem("Save As...")) {
                statusMessage_ = "Save dialog not implemented - use SaveGraph()";
            }
            if (ImGui::MenuItem("Load...")) {
                statusMessage_ = "Load dialog not implemented - use LoadGraph()";
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Close Graph")) {
                currentGraph_ = visualscripting::VisualGraph();
                editorNodes_.clear();
                editorConnections_.clear();
                selectedNodeId_.clear();
                currentFilePath_.clear();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void VisualScriptEditor::RenderNodePalette() {
    ImGui::Text("Node Palette");
    ImGui::Separator();

    auto categories = visualscripting::NodeTypeRegistry::GetAllCategories();
    for (const auto& cat : categories) {
        if (ImGui::TreeNode(cat.c_str())) {
            auto nodes = visualscripting::NodeTypeRegistry::GetDefinitionsByCategory(cat);
            for (const auto& def : nodes) {
                if (ImGui::Selectable(def.name.c_str())) {
                    ImVec2 center = ImGui::GetWindowViewport()->GetCenter();
                    AddNode(def.id, center.x - canvasOffsetX_, center.y - canvasOffsetY_);
                }
            }
            ImGui::TreePop();
        }
    }
}

void VisualScriptEditor::RenderGraphCanvas() {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();

    ImGui::InvisibleButton("Canvas", canvasSize);
    bool isHovered = ImGui::IsItemHovered();

    if (ImGui::IsWindowFocused()) {
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
            canvasOffsetX_ += ImGui::GetIO().MouseDelta.x;
            canvasOffsetY_ += ImGui::GetIO().MouseDelta.y;
        }
    }

    ImVec2 origin = ImVec2(canvasPos.x + canvasOffsetX_, canvasPos.y + canvasOffsetY_);

    for (int i = 0; i < 50; i++) {
        float x = fmodf(canvasOffsetX_, 50.0f) + i * 50.0f;
        drawList->AddLine(ImVec2(canvasPos.x + x, canvasPos.y), 
                         ImVec2(canvasPos.x + x, canvasPos.y + canvasSize.y), 
                         IM_COL32(60, 60, 60, 255));
    }
    for (int i = 0; i < 50; i++) {
        float y = fmodf(canvasOffsetY_, 50.0f) + i * 50.0f;
        drawList->AddLine(ImVec2(canvasPos.x, canvasPos.y + y), 
                         ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + y), 
                         IM_COL32(60, 60, 60, 255));
    }

    RenderConnections();

    for (auto& editorNode : editorNodes_) {
        const visualscripting::NodeDefinition* def = 
            visualscripting::NodeTypeRegistry::GetDefinition(editorNode.definitionId);
        if (!def) continue;

        ImVec2 nodePos = ImVec2(origin.x + editorNode.x, origin.y + editorNode.y);
        float nodeWidth = 180;
        float headerHeight = 25;
        float pinHeight = 20;
        float nodeHeight = headerHeight + (std::max(def->inputPins.size(), def->outputPins.size()) * pinHeight) + 20;

        bool isSelected = (editorNode.id == selectedNodeId_);

        ImU32 nodeColor = isSelected ? IM_COL32(80, 120, 200, 255) : IM_COL32(50, 50, 50, 255);
        ImU32 borderColor = isSelected ? IM_COL32(100, 150, 255, 255) : IM_COL32(80, 80, 80, 255);

        drawList->AddRectFilled(nodePos, ImVec2(nodePos.x + nodeWidth, nodePos.y + nodeHeight), nodeColor, 4);
        drawList->AddRect(nodePos, ImVec2(nodePos.x + nodeWidth, nodePos.y + nodeHeight), borderColor, 4);

        ImU32 headerColor = IM_COL32(70, 70, 70, 255);
        drawList->AddRectFilled(nodePos, ImVec2(nodePos.x + nodeWidth, nodePos.y + headerHeight), headerColor, 4, ImDrawFlags_RoundCornersTop);

        drawList->AddText(ImVec2(nodePos.x + 8, nodePos.y + 5), IM_COL32(255, 255, 255, 255), def->name.c_str());

        if (ImGui::IsMouseHoveringRect(nodePos, ImVec2(nodePos.x + nodeWidth, nodePos.y + nodeHeight))) {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                SelectNode(editorNode.id);
            }
        }

        float inputY = nodePos.y + headerHeight + 5;
        for (size_t i = 0; i < def->inputPins.size(); i++) {
            const auto& pin = def->inputPins[i];
            ImVec2 pinPos = ImVec2(nodePos.x, inputY + i * pinHeight + pinHeight / 2);
            
            ImU32 pinColor = IM_COL32(100, 200, 100, 255);
            drawList->AddCircleFilled(pinPos, 5, pinColor);
            drawList->AddCircle(pinPos, 5, IM_COL32(150, 255, 150, 255));

            drawList->AddText(ImVec2(nodePos.x + 10, inputY + i * pinHeight), 
                            IM_COL32(200, 200, 200, 255), pin.label.c_str());

            if (ImGui::IsMouseHoveringRect(ImVec2(pinPos.x - 8, pinPos.y - 8), ImVec2(pinPos.x + 8, pinPos.y + 8))) {
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    isDraggingConnection_ = true;
                    dragFromNodeId_ = editorNode.id;
                    dragFromPinId_ = pin.id;
                    dragStartX_ = pinPos.x;
                    dragStartY_ = pinPos.y;
                }
            }
        }

        float outputY = nodePos.y + headerHeight + 5;
        for (size_t i = 0; i < def->outputPins.size(); i++) {
            const auto& pin = def->outputPins[i];
            ImVec2 pinPos = ImVec2(nodePos.x + nodeWidth, outputY + i * pinHeight + pinHeight / 2);
            
            ImU32 pinColor = IM_COL32(200, 100, 100, 255);
            drawList->AddCircleFilled(pinPos, 5, pinColor);
            drawList->AddCircle(pinPos, 5, IM_COL32(255, 150, 150, 255));

            float textWidth = ImGui::CalcTextSize(pin.label.c_str()).x;
            drawList->AddText(ImVec2(nodePos.x + nodeWidth - 10 - textWidth, outputY + i * pinHeight), 
                            IM_COL32(200, 200, 200, 255), pin.label.c_str());

            if (ImGui::IsMouseHoveringRect(ImVec2(pinPos.x - 8, pinPos.y - 8), ImVec2(pinPos.x + 8, pinPos.y + 8))) {
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !isDraggingConnection_) {
                    isDraggingConnection_ = true;
                    dragFromNodeId_ = editorNode.id;
                    dragFromPinId_ = pin.id;
                    dragStartX_ = pinPos.x;
                    dragStartY_ = pinPos.y;
                }
            }
        }

        if (isDraggingConnection_) {
            ImVec2 mousePos = ImGui::GetMousePos();
            drawList->AddLine(ImVec2(dragStartX_, dragStartY_), mousePos, IM_COL32(255, 255, 100, 255), 2);
        }
    }

    if (isDraggingConnection_ && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        ImVec2 mousePos = ImGui::GetMousePos();
        for (auto& editorNode : editorNodes_) {
            const visualscripting::NodeDefinition* def = 
                visualscripting::NodeTypeRegistry::GetDefinition(editorNode.definitionId);
            if (!def) continue;

            ImVec2 nodePos = ImVec2(origin.x + editorNode.x, origin.y + editorNode.y);
            float nodeWidth = 180;
            float headerHeight = 25;
            float pinHeight = 20;

            for (size_t i = 0; i < def->inputPins.size(); i++) {
                ImVec2 pinPos = ImVec2(nodePos.x, nodePos.y + headerHeight + 5 + i * pinHeight + pinHeight / 2);
                if (mousePos.x >= pinPos.x - 10 && mousePos.x <= pinPos.x + 10 &&
                    mousePos.y >= pinPos.y - 10 && mousePos.y <= pinPos.y + 10) {
                    
                    EditorConnection conn;
                    conn.fromNodeId = dragFromNodeId_;
                    conn.fromPinId = dragFromPinId_;
                    conn.toNodeId = editorNode.id;
                    conn.toPinId = def->inputPins[i].id;
                    editorConnections_.push_back(conn);

                    for (auto& en : editorNodes_) {
                        if (en.id == dragFromNodeId_) {
                            en.outputConnections[dragFromPinId_] = editorNode.id;
                        }
                        if (en.id == editorNode.id) {
                            en.inputConnections[def->inputPins[i].id] = dragFromNodeId_;
                        }
                    }
                    break;
                }
            }
        }
        isDraggingConnection_ = false;
    }
}

void VisualScriptEditor::RenderPropertyPanel() {
    ImGui::Text("Properties");
    ImGui::Separator();

    if (selectedNodeId_.empty()) {
        ImGui::Text("No node selected");
        return;
    }

    EditorNode* selectedNode = nullptr;
    for (auto& node : editorNodes_) {
        if (node.id == selectedNodeId_) {
            selectedNode = &node;
            break;
        }
    }

    if (!selectedNode) {
        ImGui::Text("Node not found");
        return;
    }

    const visualscripting::NodeDefinition* def = 
        visualscripting::NodeTypeRegistry::GetDefinition(selectedNode->definitionId);
    if (!def) return;

    ImGui::Text("Node: %s", def->name.c_str());
    ImGui::Text("ID: %s", selectedNode->id.c_str());
    ImGui::Separator();

    char posX[64], posY[64];
    snprintf(posX, sizeof(posX), "%.0f", selectedNode->x);
    snprintf(posY, sizeof(posY), "%.0f", selectedNode->y);

    if (ImGui::InputFloat("Position X", &selectedNode->x, 10, 50, "%.0f")) {}
    if (ImGui::InputFloat("Position Y", &selectedNode->y, 10, 50, "%.0f")) {}

    ImGui::Separator();
    ImGui::Text("Input Values:");

    for (const auto& pin : def->inputPins) {
        if (pin.type == visualscripting::PinType::Float) {
            float val = 0;
            auto it = selectedNode->inputValues.find(pin.id);
            if (it != selectedNode->inputValues.end()) {
                val = std::stof(it->second);
            }
            if (ImGui::InputFloat(pin.label.c_str(), &val)) {
                selectedNode->inputValues[pin.id] = std::to_string(val);
            }
        } else if (pin.type == visualscripting::PinType::String) {
            char buf[256] = "";
            auto it = selectedNode->inputValues.find(pin.id);
            if (it != selectedNode->inputValues.end()) {
                strncpy(buf, it->second.c_str(), sizeof(buf) - 1);
            }
            if (ImGui::InputText(pin.label.c_str(), buf, sizeof(buf))) {
                selectedNode->inputValues[pin.id] = buf;
            }
        } else if (pin.type == visualscripting::PinType::Boolean) {
            bool val = false;
            auto it = selectedNode->inputValues.find(pin.id);
            if (it != selectedNode->inputValues.end()) {
                val = (it->second == "true");
            }
            if (ImGui::Checkbox(pin.label.c_str(), &val)) {
                selectedNode->inputValues[pin.id] = val ? "true" : "false";
            }
        }
    }

    ImGui::Separator();
    if (ImGui::Button("Delete Node")) {
        RemoveNode(selectedNodeId_);
        selectedNodeId_.clear();
    }
}

void VisualScriptEditor::NewGraph(const std::string& name) {
    currentGraph_.id = name;
    currentGraph_.name = name;
    editorNodes_.clear();
    editorConnections_.clear();
    selectedNodeId_.clear();
    nextNodeId_ = 1;
    statusMessage_ = "Created new graph: " + name;
}

bool VisualScriptEditor::LoadGraph(const std::string& filePath) {
    currentFilePath_ = filePath;
    visualscripting::VisualGraph graph;
    if (!visualscripting::VisualGraphSerializer::Load(graph, filePath)) {
        statusMessage_ = "Failed to load graph: " + filePath;
        return false;
    }

    currentGraph_ = graph;
    editorNodes_.clear();
    editorConnections_.clear();

    for (const auto& node : graph.nodes) {
        EditorNode editorNode;
        editorNode.id = node.id;
        editorNode.definitionId = node.definitionId;
        editorNode.title = node.title;
        editorNode.x = node.posX;
        editorNode.y = node.posY;
        editorNode.inputConnections = node.inputConnections;
        editorNode.outputConnections = node.outputConnections;
        editorNodes_.push_back(editorNode);
    }

    for (const auto& conn : graph.connections) {
        EditorConnection editorConn;
        editorConn.fromNodeId = conn.fromNodeId;
        editorConn.fromPinId = conn.fromPinId;
        editorConn.toNodeId = conn.toNodeId;
        editorConn.toPinId = conn.toPinId;
        editorConnections_.push_back(editorConn);
    }

    statusMessage_ = "Loaded graph: " + filePath;
    return true;
}

bool VisualScriptEditor::SaveGraph(const std::string& filePath) {
    currentFilePath_ = filePath;

    currentGraph_.nodes.clear();
    currentGraph_.connections.clear();

    for (const auto& editorNode : editorNodes_) {
        visualscripting::VisualGraphNode node;
        node.id = editorNode.id;
        node.definitionId = editorNode.definitionId;
        node.title = editorNode.title;
        node.posX = editorNode.x;
        node.posY = editorNode.y;
        node.inputConnections = editorNode.inputConnections;
        node.outputConnections = editorNode.outputConnections;
        currentGraph_.nodes.push_back(node);
    }

    for (const auto& editorConn : editorConnections_) {
        visualscripting::VisualGraphConnection conn;
        conn.fromNodeId = editorConn.fromNodeId;
        conn.fromPinId = editorConn.fromPinId;
        conn.toNodeId = editorConn.toNodeId;
        conn.toPinId = editorConn.toPinId;
        currentGraph_.connections.push_back(conn);
    }

    bool result = visualscripting::VisualGraphSerializer::Save(currentGraph_, filePath);
    statusMessage_ = result ? "Saved graph: " + filePath : "Failed to save graph";
    return result;
}

void VisualScriptEditor::AddNode(const std::string& definitionId, float x, float y) {
    const visualscripting::NodeDefinition* def = 
        visualscripting::NodeTypeRegistry::GetDefinition(definitionId);
    if (!def) return;

    EditorNode node;
    node.id = "node_" + std::to_string(nextNodeId_++);
    node.definitionId = definitionId;
    node.title = def->name;
    node.x = x - 90;
    node.y = y - 15;
    editorNodes_.push_back(node);

    visualscripting::VisualGraphNode graphNode;
    graphNode.id = node.id;
    graphNode.definitionId = definitionId;
    graphNode.title = def->name;
    graphNode.posX = node.x;
    graphNode.posY = node.y;
    currentGraph_.nodes.push_back(graphNode);

    statusMessage_ = "Added node: " + def->name;
}

void VisualScriptEditor::RemoveNode(const std::string& nodeId) {
    editorNodes_.erase(
        std::remove_if(editorNodes_.begin(), editorNodes_.end(),
            [&nodeId](const EditorNode& n) { return n.id == nodeId; }),
        editorNodes_.end());

    editorConnections_.erase(
        std::remove_if(editorConnections_.begin(), editorConnections_.end(),
            [&nodeId](const EditorConnection& c) { 
                return c.fromNodeId == nodeId || c.toNodeId == nodeId; 
            }),
        editorConnections_.end());

    currentGraph_.nodes.erase(
        std::remove_if(currentGraph_.nodes.begin(), currentGraph_.nodes.end(),
            [&nodeId](const visualscripting::VisualGraphNode& n) { return n.id == nodeId; }),
        currentGraph_.nodes.end());

    currentGraph_.connections.erase(
        std::remove_if(currentGraph_.connections.begin(), currentGraph_.connections.end(),
            [&nodeId](const visualscripting::VisualGraphConnection& c) { 
                return c.fromNodeId == nodeId || c.toNodeId == nodeId; 
            }),
        currentGraph_.connections.end());
}

void VisualScriptEditor::SelectNode(const std::string& nodeId) {
    selectedNodeId_ = nodeId;
    for (auto& node : editorNodes_) {
        node.selected = (node.id == nodeId);
    }
}

void VisualScriptEditor::DeselectAll() {
    selectedNodeId_.clear();
    for (auto& node : editorNodes_) {
        node.selected = false;
    }
}

void VisualScriptEditor::RenderConnections() {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 origin = ImVec2(canvasPos.x + canvasOffsetX_, canvasPos.y + canvasOffsetY_);

    for (const auto& conn : editorConnections_) {
        EditorNode* fromNode = nullptr;
        EditorNode* toNode = nullptr;

        for (auto& node : editorNodes_) {
            if (node.id == conn.fromNodeId) fromNode = &node;
            if (node.id == conn.toNodeId) toNode = &node;
        }

        if (!fromNode || !toNode) continue;

        const visualscripting::NodeDefinition* fromDef = 
            visualscripting::NodeTypeRegistry::GetDefinition(fromNode->definitionId);
        const visualscripting::NodeDefinition* toDef = 
            visualscripting::NodeTypeRegistry::GetDefinition(toNode->definitionId);

        if (!fromDef || !toDef) continue;

        int fromPinIdx = 0, toPinIdx = 0;
        for (size_t i = 0; i < fromDef->outputPins.size(); i++) {
            if (fromDef->outputPins[i].id == conn.fromPinId) { fromPinIdx = (int)i; break; }
        }
        for (size_t i = 0; i < toDef->inputPins.size(); i++) {
            if (toDef->inputPins[i].id == conn.toPinId) { toPinIdx = (int)i; break; }
        }

        ImVec2 fromPos = ImVec2(origin.x + fromNode->x + 180, origin.y + fromNode->y + 25 + 5 + fromPinIdx * 20 + 10);
        ImVec2 toPos = ImVec2(origin.x + toNode->x, origin.y + toNode->y + 25 + 5 + toPinIdx * 20 + 10);

        ImVec2 cp1 = ImVec2(fromPos.x + 50, fromPos.y);
        ImVec2 cp2 = ImVec2(toPos.x - 50, toPos.y);

        drawList->AddBezierCubic(fromPos, cp1, cp2, toPos, IM_COL32(150, 150, 150, 255), 2);
    }
}

} // namespace editor
} // namespace ge