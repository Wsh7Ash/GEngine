#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include "../math/VecTypes.h"
#include <cmath>
#include <map>

namespace ge {
namespace material {

enum class PinType {
    Float,
    Vec2,
    Vec3,
    Vec4,
    Sampler2D,
    Bool
};

enum class PinDirection {
    Input,
    Output
};

enum class NodeType {
    // Inputs
    InputFloat,
    InputColor,
    InputVector2,
    InputVector3,
    InputVector4,
    InputTexture2D,
    InputTime,
    InputUV,
    InputPosition,
    InputNormal,
    
    // Math
    MathAdd,
    MathSubtract,
    MathMultiply,
    MathDivide,
    MathPower,
    MathAbs,
    MathClamp,
    MathMix,
    MathSin,
    MathCos,
    MathDot,
    MathCross,
    MathNormalize,
    MathLength,
    MathStep,
    MathSmoothstep,
    MathFresnel,
    
    // Textures
    TextureSample,
    NormalMap,
    Triplanar,
    
    // Output
    OutputAlbedo,
    OutputMetallic,
    OutputRoughness,
    OutputNormal,
    OutputEmissive,
    OutputAlpha
};

struct Pin {
    int Id;
    std::string Name;
    PinType Type;
    PinDirection Direction;
    int NodeId;
    std::string DefaultValue;
    
    Pin() : Id(0), Type(PinType::Float), Direction(PinDirection::Input), NodeId(0) {}
    Pin(int id, const std::string& name, PinType type, PinDirection dir, int nodeId)
        : Id(id), Name(name), Type(type), Direction(dir), NodeId(nodeId) {}
    
    std::string GetTypeString() const;
    std::string GetDefaultValue() const { return DefaultValue; }
};

struct Node {
    int Id;
    NodeType Type;
    std::string Name;
    struct Position {
        float x, y;
        Position() : x(0), y(0) {}
        Position(float _x, float _y) : x(_x), y(_y) {}
    } Position;
    std::vector<Pin> Inputs;
    std::vector<Pin> Outputs;
    std::unordered_map<std::string, std::string> Properties;
    
    Node() : Id(0), Type(NodeType::InputFloat), Position(0, 0) {}
    Node(int id, NodeType type, const std::string& name, float x, float y)
        : Id(id), Type(type), Name(name), Position(x, y) {}
    
    static Node Create(NodeType type, int id, float x, float y);
};

struct Connection {
    int SourceNodeId;
    int SourcePinId;
    int TargetNodeId;
    int TargetPinId;
    
    Connection() : SourceNodeId(0), SourcePinId(0), TargetNodeId(0), TargetPinId(0) {}
    Connection(int srcNode, int srcPin, int tgtNode, int tgtPin)
        : SourceNodeId(srcNode), SourcePinId(srcPin), TargetNodeId(tgtNode), TargetPinId(tgtPin) {}
};

class MaterialGraph {
public:
    MaterialGraph();
    
    Node& AddNode(NodeType type, float x, float y);
    void RemoveNode(int nodeId);
    
    void AddConnection(const Connection& conn);
    void RemoveConnection(int nodeId, int pinId);
    
    Node* GetNode(int nodeId);
    const Node* GetNode(int nodeId) const;
    const std::vector<Node>& GetNodes() const { return nodes_; }
    const std::vector<Connection>& GetConnections() const { return connections_; }
    
    std::string GenerateGLSL() const;
    
    void Clear();
    bool IsEmpty() const { return nodes_.empty(); }
    
    void SetName(const std::string& name) { name_ = name; }
    const std::string& GetName() const { return name_; }
    
    std::string Serialize() const;
    void Deserialize(const std::string& json);
    
private:
    int nextNodeId_;
    int nextPinId_;
    std::string name_;
    std::vector<Node> nodes_;
    std::vector<Connection> connections_;
    
    Pin* FindPin(int nodeId, int pinId);
    const Pin* FindPin(int nodeId, int pinId) const;
    std::vector<Node*> GetTopologicalOrder() const;
    std::string GenerateGLSLForNode(const Node& node, std::unordered_map<int, std::string>& generated) const;
};

}
}
