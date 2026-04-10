#include "MaterialEditorPanel.h"
#include "../platform/ImGuiLayer.h"
#include <imgui.h>
#include <fstream>

namespace ge {
namespace editor {

static const float NODE_WIDTH = 150.0f;
static const float NODE_HEADER_HEIGHT = 24.0f;
static const float NODE_PIN_SIZE = 12.0f;
static const float PIN_CONNECTION_RADIUS = 8.0f;

MaterialEditorPanel::MaterialEditorPanel() {
    graph_ = std::make_shared<material::MaterialGraph>();
}

void MaterialEditorPanel::OnImGuiRender() {
    if (ImGui::Begin("Material Editor", nullptr, ImGuiWindowFlags_MenuBar)) {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New")) {
                    NewGraph();
                }
                if (ImGui::MenuItem("Open")) {
                    OpenGraph("");
                }
                if (ImGui::MenuItem("Save")) {
                    if (!currentFilepath_.empty()) {
                        SaveGraph(currentFilepath_);
                    }
                }
                if (ImGui::MenuItem("Save As")) {
                    SaveGraph("");
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("Show Code", nullptr, &showCodePreview_);
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
        
        HandleDragDrop();
        
        ImGui::Columns(2, "MaterialEditorColumns");
        
        ImGui::SetColumnWidth(0, 200.0f);
        DrawNodePalette();
        
        ImGui::NextColumn();
        DrawGraphCanvas();
        
        // ImGui::EndColumns() removed in newer ImGui version
        
        if (selectedNodeId_ >= 0) {
            ImGui::Separator();
            DrawNodeProperties();
        }
        
        if (showCodePreview_) {
            ImGui::Separator();
            DrawGeneratedCode();
        }
    }
    ImGui::End();
}

void MaterialEditorPanel::NewGraph() {
    graph_ = std::make_shared<material::MaterialGraph>();
    currentFilepath_ = "";
    selectedNodeId_ = -1;
}

void MaterialEditorPanel::OpenGraph(const std::string& filepath) {
    if (filepath.empty()) {
        return;
    }
    std::ifstream file(filepath);
    if (file.is_open()) {
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        graph_->Deserialize(content);
        currentFilepath_ = filepath;
    }
}

void MaterialEditorPanel::SaveGraph(const std::string& filepath) {
    std::string path = filepath.empty() ? currentFilepath_ : filepath;
    if (path.empty()) return;
    
    std::string content = graph_->Serialize();
    std::ofstream file(path);
    if (file.is_open()) {
        file << content;
        currentFilepath_ = path;
    }
}

void MaterialEditorPanel::DrawNodePalette() {
    ImGui::Text("Nodes");
    ImGui::Separator();
    
    if (ImGui::CollapsingHeader("Inputs", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent(10.0f);
        static const char* inputs[] = { "Float", "Color", "Vector2", "Vector3", "Vector4", 
            "Texture", "Time", "UV", "Position", "Normal" };
        static material::NodeType inputTypes[] = { 
            material::NodeType::InputFloat, material::NodeType::InputColor,
            material::NodeType::InputVector2, material::NodeType::InputVector3,
            material::NodeType::InputVector4, material::NodeType::InputTexture2D,
            material::NodeType::InputTime, material::NodeType::InputUV,
            material::NodeType::InputPosition, material::NodeType::InputNormal
        };
        
        for (int i = 0; i < 10; i++) {
            if (ImGui::Selectable(inputs[i])) {
                graph_->AddNode(inputTypes[i], 100.0f, 100.0f + i * 30.0f);
            }
        }
        ImGui::Unindent(10.0f);
    }
    
    if (ImGui::CollapsingHeader("Math", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent(10.0f);
        static const char* mathOps[] = { "Add", "Subtract", "Multiply", "Divide", 
            "Mix", "Clamp", "Sin", "Cos", "Normalize", "Power", "Abs" };
        static material::NodeType mathTypes[] = {
            material::NodeType::MathAdd, material::NodeType::MathSubtract,
            material::NodeType::MathMultiply, material::NodeType::MathDivide,
            material::NodeType::MathMix, material::NodeType::MathClamp,
            material::NodeType::MathSin, material::NodeType::MathCos,
            material::NodeType::MathNormalize, material::NodeType::MathPower,
            material::NodeType::MathAbs
        };
        
        for (int i = 0; i < 11; i++) {
            if (ImGui::Selectable(mathOps[i])) {
                graph_->AddNode(mathTypes[i], 100.0f, 100.0f + i * 30.0f);
            }
        }
        ImGui::Unindent(10.0f);
    }
    
    if (ImGui::CollapsingHeader("Textures", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent(10.0f);
        if (ImGui::Selectable("Sample Texture")) {
            graph_->AddNode(material::NodeType::TextureSample, 100.0f, 100.0f);
        }
        if (ImGui::Selectable("Normal Map")) {
            graph_->AddNode(material::NodeType::NormalMap, 100.0f, 100.0f);
        }
        ImGui::Unindent(10.0f);
    }
    
    if (ImGui::CollapsingHeader("Output", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent(10.0f);
        static const char* outputs[] = { "Albedo", "Metallic", "Roughness", "Normal", "Emissive", "Alpha" };
        static material::NodeType outputTypes[] = {
            material::NodeType::OutputAlbedo, material::NodeType::OutputMetallic,
            material::NodeType::OutputRoughness, material::NodeType::OutputNormal,
            material::NodeType::OutputEmissive, material::NodeType::OutputAlpha
        };
        
        for (int i = 0; i < 6; i++) {
            if (ImGui::Selectable(outputs[i])) {
                graph_->AddNode(outputTypes[i], 100.0f, 100.0f + i * 30.0f);
            }
        }
        ImGui::Unindent(10.0f);
    }
}

void MaterialEditorPanel::DrawGraphCanvas() {
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    
    ImGui::InvisibleButton("GraphCanvas", canvasSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    
    drawList->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(40, 40, 40, 255));
    drawList->AddRect(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(80, 80, 80, 255));
    
    ImVec2 mousePos = ImGui::GetMousePos();
    
    for (const auto& node : graph_->GetNodes()) {
        ImVec2 nodePos = ImVec2(canvasPos.x + node.Position.x, canvasPos.y + node.Position.y);
        
        float nodeHeight = NODE_HEADER_HEIGHT + node.Inputs.size() * 20.0f + node.Outputs.size() * 20.0f + 8.0f;
        
        ImU32 headerColor = IM_COL32(60, 100, 150, 255);
        if (node.Id == selectedNodeId_) {
            headerColor = IM_COL32(80, 130, 180, 255);
        }
        
        drawList->AddRectFilled(nodePos, ImVec2(nodePos.x + NODE_WIDTH, nodePos.y + nodeHeight), IM_COL32(50, 50, 50, 255));
        drawList->AddRectFilled(nodePos, ImVec2(nodePos.x + NODE_WIDTH, nodePos.y + NODE_HEADER_HEIGHT), headerColor);
        drawList->AddRect(nodePos, ImVec2(nodePos.x + NODE_WIDTH, nodePos.y + nodeHeight), IM_COL32(100, 100, 100, 255));
        
        ImVec2 textPos = ImVec2(nodePos.x + 8, nodePos.y + 6);
        drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), node.Name.c_str());
        
        float pinY = nodePos.y + NODE_HEADER_HEIGHT + 8.0f;
        
        for (const auto& input : node.Inputs) {
            ImVec2 pinPos = ImVec2(nodePos.x, pinY);
            drawList->AddCircleFilled(pinPos, PIN_CONNECTION_RADIUS, IM_COL32(200, 200, 100, 255));
            
            ImVec2 labelPos = ImVec2(nodePos.x + 16, pinY - 4);
            drawList->AddText(labelPos, IM_COL32(200, 200, 200, 255), input.Name.c_str());
            
            pinY += 20.0f;
        }
        
        pinY = nodePos.y + NODE_HEADER_HEIGHT + 8.0f;
        for (const auto& output : node.Outputs) {
            ImVec2 pinPos = ImVec2(nodePos.x + NODE_WIDTH, pinY);
            drawList->AddCircleFilled(pinPos, PIN_CONNECTION_RADIUS, IM_COL32(100, 200, 100, 255));
            
            ImVec2 labelPos = ImVec2(nodePos.x + NODE_WIDTH - 60, pinY - 4);
            drawList->AddText(labelPos, IM_COL32(200, 200, 200, 255), output.Name.c_str());
            
            pinY += 20.0f;
        }
    }
    
    for (const auto& conn : graph_->GetConnections()) {
        material::Node* srcNode = graph_->GetNode(conn.SourceNodeId);
        material::Node* dstNode = graph_->GetNode(conn.TargetNodeId);
        
        if (!srcNode || !dstNode) continue;
        
        ImVec2 srcPos = ImVec2(canvasPos.x + srcNode->Position.x + NODE_WIDTH, 
                               canvasPos.y + srcNode->Position.y + NODE_HEADER_HEIGHT + 8.0f);
        
        for (size_t i = 0; i < srcNode->Outputs.size(); i++) {
            if (srcNode->Outputs[i].Id == conn.SourcePinId) {
                srcPos.y += i * 20.0f;
                break;
            }
        }
        
        ImVec2 dstPos = ImVec2(canvasPos.x + dstNode->Position.x,
                               canvasPos.y + dstNode->Position.y + NODE_HEADER_HEIGHT + 8.0f);
        
        for (size_t i = 0; i < dstNode->Inputs.size(); i++) {
            if (dstNode->Inputs[i].Id == conn.TargetPinId) {
                dstPos.y += i * 20.0f;
                break;
            }
        }
        
        drawList->AddLine(srcPos, dstPos, IM_COL32(150, 150, 150, 255), 2.0f);
    }
    
    if (isDraggingConnection_ && connectionStartNodeId_ >= 0) {
        material::Node* startNode = graph_->GetNode(connectionStartNodeId_);
        if (startNode) {
            ImVec2 startPos;
            if (connectionStartPinId_ >= 0) {
                startPos = ImVec2(canvasPos.x + startNode->Position.x + NODE_WIDTH,
                                 canvasPos.y + startNode->Position.y + NODE_HEADER_HEIGHT + 8.0f);
                for (size_t i = 0; i < startNode->Outputs.size(); i++) {
                    if ((int)startNode->Outputs[i].Id == connectionStartPinId_) {
                        startPos.y += i * 20.0f;
                        break;
                    }
                }
            } else {
                startPos = ImVec2(canvasPos.x + startNode->Position.x,
                                 canvasPos.y + startNode->Position.y + NODE_HEADER_HEIGHT + 8.0f);
                for (size_t i = 0; i < startNode->Inputs.size(); i++) {
                    if ((int)startNode->Inputs[i].Id == -connectionStartPinId_) {
                        startPos.y += i * 20.0f;
                        break;
                    }
                }
            }
            
            drawList->AddLine(startPos, mousePos, IM_COL32(200, 200, 100, 255), 2.0f);
        }
    }
    
    if (ImGui::IsMouseDown(0) && !ImGui::GetIO().WantCaptureMouse) {
        for (const auto& node : graph_->GetNodes()) {
            ImVec2 nodePos = ImVec2(canvasPos.x + node.Position.x, canvasPos.y + node.Position.y);
            ImVec2 nodeSize = ImVec2(NODE_WIDTH, NODE_HEADER_HEIGHT + node.Inputs.size() * 20.0f + node.Outputs.size() * 20.0f + 8.0f);
            
            if (mousePos.x >= nodePos.x && mousePos.x <= nodePos.x + nodeSize.x &&
                mousePos.y >= nodePos.y && mousePos.y <= nodePos.y + nodeSize.y) {
                dragNodeId_ = node.Id;
                dragOffset_.x = mousePos.x - nodePos.x;
                dragOffset_.y = mousePos.y - nodePos.y;
                break;
            }
        }
    }
    
    if (dragNodeId_ >= 0 && ImGui::IsMouseDown(0)) {
        material::Node* node = graph_->GetNode(dragNodeId_);
        if (node) {
            node->Position.x = mousePos.x - canvasPos.x - dragOffset_.x;
            node->Position.y = mousePos.y - canvasPos.y - dragOffset_.y;
        }
    } else {
        dragNodeId_ = -1;
    }
    
    if (ImGui::IsMouseClicked(1)) {
        for (const auto& node : graph_->GetNodes()) {
            ImVec2 nodePos = ImVec2(canvasPos.x + node.Position.x, canvasPos.y + node.Position.y);
            ImVec2 nodeSize = ImVec2(NODE_WIDTH, NODE_HEADER_HEIGHT + node.Inputs.size() * 20.0f + node.Outputs.size() * 20.0f + 8.0f);
            
            if (mousePos.x >= nodePos.x && mousePos.x <= nodePos.x + nodeSize.x &&
                mousePos.y >= nodePos.y && mousePos.y <= nodePos.y + nodeSize.y) {
                selectedNodeId_ = node.Id;
                break;
            }
        }
    }
    
    if (ImGui::IsMouseClicked(0)) {
        for (const auto& node : graph_->GetNodes()) {
            ImVec2 nodePos = ImVec2(canvasPos.x + node.Position.x, canvasPos.y + node.Position.y);
            
            float pinY = nodePos.y + NODE_HEADER_HEIGHT + 8.0f;
            for (const auto& output : node.Outputs) {
                ImVec2 pinPos = ImVec2(nodePos.x + NODE_WIDTH, pinY);
                if ((mousePos.x - pinPos.x) * (mousePos.x - pinPos.x) + 
                    (mousePos.y - pinPos.y) * (mousePos.y - pinPos.y) < 100.0f) {
                    BeginConnection(node.Id, output.Id);
                    break;
                }
                pinY += 20.0f;
            }
            
            pinY = nodePos.y + NODE_HEADER_HEIGHT + 8.0f;
            for (const auto& input : node.Inputs) {
                ImVec2 pinPos = ImVec2(nodePos.x, pinY);
                if ((mousePos.x - pinPos.x) * (mousePos.x - pinPos.x) + 
                    (mousePos.y - pinPos.y) * (mousePos.y - pinPos.y) < 100.0f) {
                    EndConnection(node.Id, -input.Id);
                    break;
                }
                pinY += 20.0f;
            }
        }
    }
    
    if (ImGui::IsMouseReleased(0) && isDraggingConnection_) {
        isDraggingConnection_ = false;
        connectionStartNodeId_ = -1;
        connectionStartPinId_ = -1;
    }
}

void MaterialEditorPanel::HandleDragDrop() {
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
        ImGui::SetDragDropPayload("NODE_TYPE", nullptr, 0);
        ImGui::EndDragDropSource();
    }
}

void MaterialEditorPanel::BeginConnection(int nodeId, int pinId) {
    connectionStartNodeId_ = nodeId;
    connectionStartPinId_ = pinId;
    isDraggingConnection_ = true;
}

void MaterialEditorPanel::EndConnection(int nodeId, int pinId) {
    if (connectionStartNodeId_ >= 0 && connectionStartPinId_ >= 0) {
        material::Connection conn(connectionStartNodeId_, connectionStartPinId_, nodeId, -pinId);
        graph_->AddConnection(conn);
    }
    isDraggingConnection_ = false;
    connectionStartNodeId_ = -1;
    connectionStartPinId_ = -1;
}

void MaterialEditorPanel::DrawNodeProperties() {
    if (selectedNodeId_ < 0) return;
    
    material::Node* node = graph_->GetNode(selectedNodeId_);
    if (!node) return;
    
    ImGui::Text("Properties: %s", node->Name.c_str());
    ImGui::Separator();
    
    for (auto& prop : node->Properties) {
        char buffer[256];
        strncpy(buffer, prop.second.c_str(), sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = 0;
        
        if (ImGui::InputText(prop.first.c_str(), buffer, sizeof(buffer))) {
            prop.second = buffer;
        }
    }
    
    if (ImGui::Button("Delete Node")) {
        graph_->RemoveNode(selectedNodeId_);
        selectedNodeId_ = -1;
    }
}

void MaterialEditorPanel::DrawGeneratedCode() {
    ImGui::Text("Generated GLSL");
    ImGui::Separator();
    
    std::string code = graph_->GenerateGLSL();
    ImGui::TextWrapped("%s", code.c_str());
    
    if (ImGui::Button("Compile")) {
        generatedCode_ = code;
    }
}

bool MaterialEditorPanel::IsPinHovered(int nodeId, int pinId, const ImVec2& mousePos) {
    material::Node* node = graph_->GetNode(nodeId);
    if (!node) return false;
    
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 nodePos = ImVec2(canvasPos.x + node->Position.x, canvasPos.y + node->Position.y);
    
    float pinY = nodePos.y + NODE_HEADER_HEIGHT + 8.0f;
    
    for (const auto& output : node->Outputs) {
        ImVec2 pinPos = ImVec2(nodePos.x + NODE_WIDTH, pinY);
        if ((mousePos.x - pinPos.x) * (mousePos.x - pinPos.x) + 
            (mousePos.y - pinPos.y) * (mousePos.y - pinPos.y) < 100.0f) {
            return true;
        }
        pinY += 20.0f;
    }
    
    return false;
}

}
}
