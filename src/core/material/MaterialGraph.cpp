#include "MaterialGraph.h"
#include <sstream>
#include <algorithm>
#include <stack>
#include <cstdint>

namespace ge {
namespace material {

MaterialGraph::MaterialGraph() : nextNodeId_(1), nextPinId_(1), name_("New Material") {}

std::string Pin::GetTypeString() const {
    switch (Type) {
        case PinType::Float: return "float";
        case PinType::Vec2: return "vec2";
        case PinType::Vec3: return "vec3";
        case PinType::Vec4: return "vec4";
        case PinType::Sampler2D: return "sampler2D";
        case PinType::Bool: return "bool";
        default: return "float";
    }
}

Node Node::Create(NodeType type, int id, float x, float y) {
    Node node;
    node.Id = id;
    node.Type = type;
    node.Position = {x, y};
    
    switch (type) {
        case NodeType::InputFloat:
            node.Name = "Float";
            node.Outputs.emplace_back(0, "Value", PinType::Float, PinDirection::Output, id);
            node.Properties["Value"] = "0.5";
            node.Properties["Min"] = "0.0";
            node.Properties["Max"] = "1.0";
            break;
        case NodeType::InputColor:
            node.Name = "Color";
            node.Outputs.emplace_back(0, "Color", PinType::Vec4, PinDirection::Output, id);
            node.Properties["R"] = "1.0";
            node.Properties["G"] = "1.0";
            node.Properties["B"] = "1.0";
            node.Properties["A"] = "1.0";
            break;
        case NodeType::InputVector2:
            node.Name = "Vector2";
            node.Outputs.emplace_back(0, "Value", PinType::Vec2, PinDirection::Output, id);
            node.Properties["X"] = "0.0";
            node.Properties["Y"] = "0.0";
            break;
        case NodeType::InputVector3:
            node.Name = "Vector3";
            node.Outputs.emplace_back(0, "Value", PinType::Vec3, PinDirection::Output, id);
            node.Properties["X"] = "0.0";
            node.Properties["Y"] = "0.0";
            node.Properties["Z"] = "0.0";
            break;
        case NodeType::InputVector4:
            node.Name = "Vector4";
            node.Outputs.emplace_back(0, "Value", PinType::Vec4, PinDirection::Output, id);
            node.Properties["X"] = "0.0";
            node.Properties["Y"] = "0.0";
            node.Properties["Z"] = "0.0";
            node.Properties["W"] = "1.0";
            break;
        case NodeType::InputTexture2D:
            node.Name = "Texture";
            node.Outputs.emplace_back(0, "RGBA", PinType::Vec4, PinDirection::Output, id);
            node.Properties["Path"] = "";
            break;
        case NodeType::InputTime:
            node.Name = "Time";
            node.Outputs.emplace_back(0, "Time", PinType::Float, PinDirection::Output, id);
            break;
        case NodeType::InputUV:
            node.Name = "UV";
            node.Outputs.emplace_back(0, "UV", PinType::Vec2, PinDirection::Output, id);
            break;
        case NodeType::InputPosition:
            node.Name = "Position";
            node.Outputs.emplace_back(0, "Position", PinType::Vec3, PinDirection::Output, id);
            break;
        case NodeType::InputNormal:
            node.Name = "Normal";
            node.Outputs.emplace_back(0, "Normal", PinType::Vec3, PinDirection::Output, id);
            break;
        case NodeType::MathAdd:
            node.Name = "Add";
            node.Inputs.emplace_back(0, "A", PinType::Vec4, PinDirection::Input, id);
            node.Inputs.emplace_back(1, "B", PinType::Vec4, PinDirection::Input, id);
            node.Outputs.emplace_back(2, "Result", PinType::Vec4, PinDirection::Output, id);
            break;
        case NodeType::MathSubtract:
            node.Name = "Subtract";
            node.Inputs.emplace_back(0, "A", PinType::Vec4, PinDirection::Input, id);
            node.Inputs.emplace_back(1, "B", PinType::Vec4, PinDirection::Input, id);
            node.Outputs.emplace_back(2, "Result", PinType::Vec4, PinDirection::Output, id);
            break;
        case NodeType::MathMultiply:
            node.Name = "Multiply";
            node.Inputs.emplace_back(0, "A", PinType::Vec4, PinDirection::Input, id);
            node.Inputs.emplace_back(1, "B", PinType::Vec4, PinDirection::Input, id);
            node.Outputs.emplace_back(2, "Result", PinType::Vec4, PinDirection::Output, id);
            break;
        case NodeType::MathDivide:
            node.Name = "Divide";
            node.Inputs.emplace_back(0, "A", PinType::Vec4, PinDirection::Input, id);
            node.Inputs.emplace_back(1, "B", PinType::Vec4, PinDirection::Input, id);
            node.Outputs.emplace_back(2, "Result", PinType::Vec4, PinDirection::Output, id);
            break;
        case NodeType::MathPower:
            node.Name = "Power";
            node.Inputs.emplace_back(0, "Base", PinType::Float, PinDirection::Input, id);
            node.Inputs.emplace_back(1, "Exp", PinType::Float, PinDirection::Input, id);
            node.Outputs.emplace_back(2, "Result", PinType::Float, PinDirection::Output, id);
            break;
        case NodeType::MathAbs:
            node.Name = "Abs";
            node.Inputs.emplace_back(0, "Value", PinType::Vec4, PinDirection::Input, id);
            node.Outputs.emplace_back(1, "Result", PinType::Vec4, PinDirection::Output, id);
            break;
        case NodeType::MathClamp:
            node.Name = "Clamp";
            node.Inputs.emplace_back(0, "Value", PinType::Float, PinDirection::Input, id);
            node.Inputs.emplace_back(1, "Min", PinType::Float, PinDirection::Input, id);
            node.Inputs.emplace_back(2, "Max", PinType::Float, PinDirection::Input, id);
            node.Outputs.emplace_back(3, "Result", PinType::Float, PinDirection::Output, id);
            break;
        case NodeType::MathMix:
            node.Name = "Mix";
            node.Inputs.emplace_back(0, "A", PinType::Vec4, PinDirection::Input, id);
            node.Inputs.emplace_back(1, "B", PinType::Vec4, PinDirection::Input, id);
            node.Inputs.emplace_back(2, "T", PinType::Float, PinDirection::Input, id);
            node.Outputs.emplace_back(3, "Result", PinType::Vec4, PinDirection::Output, id);
            break;
        case NodeType::MathSin:
            node.Name = "Sin";
            node.Inputs.emplace_back(0, "Value", PinType::Float, PinDirection::Input, id);
            node.Outputs.emplace_back(1, "Result", PinType::Float, PinDirection::Output, id);
            break;
        case NodeType::MathCos:
            node.Name = "Cos";
            node.Inputs.emplace_back(0, "Value", PinType::Float, PinDirection::Input, id);
            node.Outputs.emplace_back(1, "Result", PinType::Float, PinDirection::Output, id);
            break;
        case NodeType::MathDot:
            node.Name = "Dot";
            node.Inputs.emplace_back(0, "A", PinType::Vec3, PinDirection::Input, id);
            node.Inputs.emplace_back(1, "B", PinType::Vec3, PinDirection::Input, id);
            node.Outputs.emplace_back(2, "Result", PinType::Float, PinDirection::Output, id);
            break;
        case NodeType::MathCross:
            node.Name = "Cross";
            node.Inputs.emplace_back(0, "A", PinType::Vec3, PinDirection::Input, id);
            node.Inputs.emplace_back(1, "B", PinType::Vec3, PinDirection::Input, id);
            node.Outputs.emplace_back(2, "Result", PinType::Vec3, PinDirection::Output, id);
            break;
        case NodeType::MathNormalize:
            node.Name = "Normalize";
            node.Inputs.emplace_back(0, "Value", PinType::Vec3, PinDirection::Input, id);
            node.Outputs.emplace_back(1, "Result", PinType::Vec3, PinDirection::Output, id);
            break;
        case NodeType::MathLength:
            node.Name = "Length";
            node.Inputs.emplace_back(0, "Value", PinType::Vec3, PinDirection::Input, id);
            node.Outputs.emplace_back(1, "Result", PinType::Float, PinDirection::Output, id);
            break;
        case NodeType::MathStep:
            node.Name = "Step";
            node.Inputs.emplace_back(0, "Edge", PinType::Float, PinDirection::Input, id);
            node.Inputs.emplace_back(1, "Value", PinType::Float, PinDirection::Input, id);
            node.Outputs.emplace_back(2, "Result", PinType::Float, PinDirection::Output, id);
            break;
        case NodeType::MathSmoothstep:
            node.Name = "Smoothstep";
            node.Inputs.emplace_back(0, "Edge0", PinType::Float, PinDirection::Input, id);
            node.Inputs.emplace_back(1, "Edge1", PinType::Float, PinDirection::Input, id);
            node.Inputs.emplace_back(2, "Value", PinType::Float, PinDirection::Input, id);
            node.Outputs.emplace_back(3, "Result", PinType::Float, PinDirection::Output, id);
            break;
        case NodeType::MathFresnel:
            node.Name = "Fresnel";
            node.Inputs.emplace_back(0, "Normal", PinType::Vec3, PinDirection::Input, id);
            node.Inputs.emplace_back(1, "ViewDir", PinType::Vec3, PinDirection::Input, id);
            node.Inputs.emplace_back(2, "Power", PinType::Float, PinDirection::Input, id);
            node.Outputs.emplace_back(3, "Result", PinType::Float, PinDirection::Output, id);
            break;
        case NodeType::TextureSample:
            node.Name = "Sample Texture";
            node.Inputs.emplace_back(0, "UV", PinType::Vec2, PinDirection::Input, id);
            node.Inputs.emplace_back(1, "Texture", PinType::Sampler2D, PinDirection::Input, id);
            node.Outputs.emplace_back(2, "RGBA", PinType::Vec4, PinDirection::Output, id);
            node.Outputs.emplace_back(3, "R", PinType::Float, PinDirection::Output, id);
            node.Outputs.emplace_back(4, "G", PinType::Float, PinDirection::Output, id);
            node.Outputs.emplace_back(5, "B", PinType::Float, PinDirection::Output, id);
            node.Outputs.emplace_back(6, "A", PinType::Float, PinDirection::Output, id);
            break;
        case NodeType::NormalMap:
            node.Name = "Normal Map";
            node.Inputs.emplace_back(0, "UV", PinType::Vec2, PinDirection::Input, id);
            node.Inputs.emplace_back(1, "NormalTex", PinType::Sampler2D, PinDirection::Input, id);
            node.Inputs.emplace_back(2, "Strength", PinType::Float, PinDirection::Input, id);
            node.Outputs.emplace_back(3, "Normal", PinType::Vec3, PinDirection::Output, id);
            break;
        case NodeType::Triplanar:
            node.Name = "Triplanar";
            node.Inputs.emplace_back(0, "Position", PinType::Vec3, PinDirection::Input, id);
            node.Inputs.emplace_back(1, "Texture", PinType::Sampler2D, PinDirection::Input, id);
            node.Inputs.emplace_back(2, "Blend", PinType::Float, PinDirection::Input, id);
            node.Outputs.emplace_back(3, "Color", PinType::Vec4, PinDirection::Output, id);
            break;
        case NodeType::OutputAlbedo:
            node.Name = "Albedo";
            node.Inputs.emplace_back(0, "Color", PinType::Vec4, PinDirection::Input, id);
            node.Properties["UseAlpha"] = "false";
            break;
        case NodeType::OutputMetallic:
            node.Name = "Metallic";
            node.Inputs.emplace_back(0, "Value", PinType::Float, PinDirection::Input, id);
            break;
        case NodeType::OutputRoughness:
            node.Name = "Roughness";
            node.Inputs.emplace_back(0, "Value", PinType::Float, PinDirection::Input, id);
            break;
        case NodeType::OutputNormal:
            node.Name = "Normal";
            node.Inputs.emplace_back(0, "Normal", PinType::Vec3, PinDirection::Input, id);
            break;
        case NodeType::OutputEmissive:
            node.Name = "Emissive";
            node.Inputs.emplace_back(0, "Color", PinType::Vec4, PinDirection::Input, id);
            break;
        case NodeType::OutputAlpha:
            node.Name = "Alpha";
            node.Inputs.emplace_back(0, "Value", PinType::Float, PinDirection::Input, id);
            break;
    }
    return node;
}

Node& MaterialGraph::AddNode(NodeType type, float x, float y) {
    int id = nextNodeId_++;
    nodes_.push_back(Node::Create(type, id, x, y));
    return nodes_.back();
}

void MaterialGraph::RemoveNode(int nodeId) {
    nodes_.erase(std::remove_if(nodes_.begin(), nodes_.end(), 
        [nodeId](const Node& n) { return n.Id == nodeId; }), nodes_.end());
    connections_.erase(std::remove_if(connections_.begin(), connections_.end(),
        [nodeId](const Connection& c) { 
            return c.SourceNodeId == nodeId || c.TargetNodeId == nodeId; 
        }), connections_.end());
}

void MaterialGraph::AddConnection(const Connection& conn) {
    RemoveConnection(conn.TargetNodeId, conn.TargetPinId);
    connections_.push_back(conn);
}

void MaterialGraph::RemoveConnection(int nodeId, int pinId) {
    connections_.erase(std::remove_if(connections_.begin(), connections_.end(),
        [nodeId, pinId](const Connection& c) {
            return c.TargetNodeId == nodeId && c.TargetPinId == pinId;
        }), connections_.end());
}

Node* MaterialGraph::GetNode(int nodeId) {
    for (auto& node : nodes_) {
        if (node.Id == nodeId) return &node;
    }
    return nullptr;
}

Pin* MaterialGraph::FindPin(int nodeId, int pinId) {
    Node* node = GetNode(nodeId);
    if (!node) return nullptr;
    for (auto& pin : node->Inputs) {
        if (pin.Id == pinId) return &pin;
    }
    for (auto& pin : node->Outputs) {
        if (pin.Id == pinId) return &pin;
    }
    return nullptr;
}

const Pin* MaterialGraph::FindPin(int nodeId, int pinId) const {
    for (const auto& node : nodes_) {
        if (node.Id == nodeId) {
            for (const auto& pin : node.Inputs) {
                if (pin.Id == pinId) return &pin;
            }
            for (const auto& pin : node.Outputs) {
                if (pin.Id == pinId) return &pin;
            }
        }
    }
    return nullptr;
}

std::vector<Node*> MaterialGraph::GetTopologicalOrder() const {
    std::unordered_map<int, int> inDegree;
    std::unordered_map<int, std::vector<int>> adj;
    
    for (const auto& node : nodes_) {
        inDegree[node.Id] = 0;
    }
    
    std::stack<int> s;
    for (const auto& node : nodes_) {
        if (inDegree[node.Id] == 0) {
            s.push(node.Id);
        }
    }
    
    std::vector<Node*> result;
    while (!s.empty()) {
        int nodeId = s.top();
        s.pop();
        
        if (Node* node = const_cast<MaterialGraph*>(this)->GetNode(nodeId)) {
            result.push_back(node);
        }
        
        for (int neighbor : adj[nodeId]) {
            inDegree[neighbor]--;
            if (inDegree[neighbor] == 0) {
                s.push(neighbor);
            }
        }
    }
    
    return result;
}

std::string MaterialGraph::GenerateGLSLForNode(const Node& node, std::unordered_map<int, std::string>& generated) const {
    if (generated.find(node.Id) != generated.end()) {
        return "node_" + std::to_string(node.Id);
    }
    
    std::string result;
    std::string varName = "node_" + std::to_string(node.Id);
    
    switch (node.Type) {
        case NodeType::InputFloat: {
            std::string val = node.Properties.at("Value");
            result = "float " + varName + " = " + val + ";";
            break;
        }
        case NodeType::InputColor: {
            std::string r = node.Properties.at("R");
            std::string g = node.Properties.at("G");
            std::string b = node.Properties.at("B");
            std::string a = node.Properties.at("A");
            result = "vec4 " + varName + " = vec4(" + r + ", " + g + ", " + b + ", " + a + ");";
            break;
        }
        case NodeType::InputVector2: {
            std::string x = node.Properties.at("X");
            std::string y = node.Properties.at("Y");
            result = "vec2 " + varName + " = vec2(" + x + ", " + y + ");";
            break;
        }
        case NodeType::InputVector3: {
            std::string x = node.Properties.at("X");
            std::string y = node.Properties.at("Y");
            std::string z = node.Properties.at("Z");
            result = "vec3 " + varName + " = vec3(" + x + ", " + y + ", " + z + ");";
            break;
        }
        case NodeType::InputVector4: {
            std::string x = node.Properties.at("X");
            std::string y = node.Properties.at("Y");
            std::string z = node.Properties.at("Z");
            std::string w = node.Properties.at("W");
            result = "vec4 " + varName + " = vec4(" + x + ", " + y + ", " + z + ", " + w + ");";
            break;
        }
        case NodeType::InputTexture2D: {
            std::string path = node.Properties.at("Path");
            result = "sampler2D " + varName + "; // " + path;
            break;
        }
        case NodeType::InputTime: {
            result = "float " + varName + " = u_Time;";
            break;
        }
        case NodeType::InputUV: {
            result = "vec2 " + varName + " = v_TexCoord;";
            break;
        }
        case NodeType::InputPosition: {
            result = "vec3 " + varName + " = v_FragPos;";
            break;
        }
        case NodeType::InputNormal: {
            result = "vec3 " + varName + " = v_Normal;";
            break;
        }
        case NodeType::MathAdd: {
            std::string a = "node_" + std::to_string(node.Inputs[0].NodeId);
            std::string b = "node_" + std::to_string(node.Inputs[1].NodeId);
            result = "vec4 " + varName + " = " + a + " + " + b + ";";
            break;
        }
        case NodeType::MathSubtract: {
            std::string a = "node_" + std::to_string(node.Inputs[0].NodeId);
            std::string b = "node_" + std::to_string(node.Inputs[1].NodeId);
            result = "vec4 " + varName + " = " + a + " - " + b + ";";
            break;
        }
        case NodeType::MathMultiply: {
            std::string a = "node_" + std::to_string(node.Inputs[0].NodeId);
            std::string b = "node_" + std::to_string(node.Inputs[1].NodeId);
            result = "vec4 " + varName + " = " + a + " * " + b + ";";
            break;
        }
        case NodeType::MathDivide: {
            std::string a = "node_" + std::to_string(node.Inputs[0].NodeId);
            std::string b = "node_" + std::to_string(node.Inputs[1].NodeId);
            result = "vec4 " + varName + " = " + a + " / " + b + ";";
            break;
        }
        case NodeType::MathMix: {
            std::string a = "node_" + std::to_string(node.Inputs[0].NodeId);
            std::string b = "node_" + std::to_string(node.Inputs[1].NodeId);
            std::string t = "node_" + std::to_string(node.Inputs[2].NodeId);
            result = "vec4 " + varName + " = mix(" + a + ", " + b + ", " + t + ");";
            break;
        }
        case NodeType::MathSin: {
            std::string v = "node_" + std::to_string(node.Inputs[0].NodeId);
            result = "float " + varName + " = sin(" + v + ");";
            break;
        }
        case NodeType::MathCos: {
            std::string v = "node_" + std::to_string(node.Inputs[0].NodeId);
            result = "float " + varName + " = cos(" + v + ");";
            break;
        }
        case NodeType::MathNormalize: {
            std::string v = "node_" + std::to_string(node.Inputs[0].NodeId);
            result = "vec3 " + varName + " = normalize(" + v + ");";
            break;
        }
        case NodeType::MathFresnel: {
            std::string n = "node_" + std::to_string(node.Inputs[0].NodeId);
            std::string v = "node_" + std::to_string(node.Inputs[1].NodeId);
            std::string p = "node_" + std::to_string(node.Inputs[2].NodeId);
            result = "float " + varName + " = pow(1.0 - max(dot(" + n + ", " + v + "), 0.0), " + p + ");";
            break;
        }
        case NodeType::TextureSample: {
            std::string uv = "node_" + std::to_string(node.Inputs[0].NodeId);
            std::string tex = "node_" + std::to_string(node.Inputs[1].NodeId);
            result = "vec4 " + varName + " = texture(" + tex + ", " + uv + ");";
            break;
        }
        case NodeType::NormalMap: {
            std::string uv = "node_" + std::to_string(node.Inputs[0].NodeId);
            std::string nt = "node_" + std::to_string(node.Inputs[1].NodeId);
            std::string s = "node_" + std::to_string(node.Inputs[2].NodeId);
            result = "vec3 " + varName + " = texture(" + nt + ", " + uv + ").rgb * " + s + ";";
            break;
        }
        case NodeType::OutputAlbedo:
        case NodeType::OutputMetallic:
        case NodeType::OutputRoughness:
        case NodeType::OutputNormal:
        case NodeType::OutputEmissive:
        case NodeType::OutputAlpha:
            result = "// Output node: " + node.Name;
            break;
        default:
            result = "// Unsupported node type";
    }
    
    generated[node.Id] = varName;
    return result;
}

std::string MaterialGraph::GenerateGLSL() const {
    std::ostringstream oss;
    
    oss << "#version 450 core\n\n";
    oss << "in vec2 v_TexCoord;\n";
    oss << "in vec3 v_FragPos;\n";
    oss << "in vec3 v_Normal;\n";
    oss << "uniform float u_Time;\n";
    oss << "\n";
    
    std::unordered_map<int, std::string> generated;
    auto sortedNodes = GetTopologicalOrder();
    
    for (auto* node : sortedNodes) {
        if (node) {
            oss << GenerateGLSLForNode(*node, generated) << "\n";
        }
    }
    
    oss << "\nvoid main() {\n";
    
    for (const auto& node : nodes_) {
        if (node.Type == NodeType::OutputAlbedo) {
            if (!node.Inputs.empty()) {
                oss << "    vec3 u_AlbedoColor = node_" << node.Inputs[0].NodeId << ".rgb;\n";
            }
        } else if (node.Type == NodeType::OutputMetallic) {
            if (!node.Inputs.empty()) {
                oss << "    float u_Metallic = node_" << node.Inputs[0].NodeId << ";\n";
            }
        } else if (node.Type == NodeType::OutputRoughness) {
            if (!node.Inputs.empty()) {
                oss << "    float u_Roughness = node_" << node.Inputs[0].NodeId << ";\n";
            }
        } else if (node.Type == NodeType::OutputNormal) {
            if (!node.Inputs.empty()) {
                oss << "    vec3 u_Normal = node_" << node.Inputs[0].NodeId << ";\n";
            }
        } else if (node.Type == NodeType::OutputEmissive) {
            if (!node.Inputs.empty()) {
                oss << "    vec3 u_Emissive = node_" << node.Inputs[0].NodeId << ".rgb;\n";
            }
        }
    }
    
    oss << "}\n";
    
    return oss.str();
}

void MaterialGraph::Clear() {
    nodes_.clear();
    connections_.clear();
    nextNodeId_ = 1;
    nextPinId_ = 1;
}

std::string MaterialGraph::Serialize() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"name\": \"" << name_ << "\",\n";
    oss << "  \"nodes\": [\n";
    for (size_t i = 0; i < nodes_.size(); i++) {
        const auto& node = nodes_[i];
        oss << "    {\n";
        oss << "      \"id\": " << node.Id << ",\n";
        oss << "      \"type\": " << static_cast<int>(node.Type) << ",\n";
        oss << "      \"x\": " << node.Position.x << ",\n";
        oss << "      \"y\": " << node.Position.y << ",\n";
        oss << "      \"properties\": {";
        bool first = true;
        for (const auto& prop : node.Properties) {
            if (!first) oss << ", ";
            oss << "\"" << prop.first << "\": \"" << prop.second << "\"";
            first = false;
        }
        oss << "}\n";
        oss << "    }";
        if (i < nodes_.size() - 1) oss << ",";
        oss << "\n";
    }
    oss << "  ],\n";
    oss << "  \"connections\": [\n";
    for (size_t i = 0; i < connections_.size(); i++) {
        const auto& conn = connections_[i];
        oss << "    {" << conn.SourceNodeId << ", " << conn.SourcePinId << ", "
            << conn.TargetNodeId << ", " << conn.TargetPinId << "}";
        if (i < connections_.size() - 1) oss << ",";
        oss << "\n";
    }
    oss << "  ]\n";
    oss << "}\n";
    return oss.str();
}

void MaterialGraph::Deserialize(const std::string& jsonStr) {
    Clear();
    
    size_t namePos = jsonStr.find("\"name\":");
    if (namePos != std::string::npos) {
        size_t start = jsonStr.find("\"", namePos + 7);
        size_t end = jsonStr.find("\"", start + 1);
        if (start != std::string::npos && end != std::string::npos) {
            name_ = jsonStr.substr(start + 1, end - start - 1);
        }
    }
}

}
}
