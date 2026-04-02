#pragma once

#include "../../visualscripting/VisualGraph.h"
#include "../../visualscripting/NodeTypeRegistry.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace ge {
namespace editor {

struct EditorNode {
    std::string id;
    std::string definitionId;
    std::string title;
    float x = 0, y = 0;
    bool selected = false;
    bool collapsed = false;
    std::unordered_map<std::string, std::string> inputValues;
    std::unordered_map<std::string, std::string> inputConnections;
    std::unordered_map<std::string, std::string> outputConnections;
};

struct EditorConnection {
    std::string fromNodeId;
    std::string fromPinId;
    std::string toNodeId;
    std::string toPinId;
};

class VisualScriptEditor {
public:
    static void Init();
    static void Shutdown();
    static void Render();

    static void NewGraph(const std::string& name);
    static bool LoadGraph(const std::string& filePath);
    static bool SaveGraph(const std::string& filePath);

    static visualscripting::VisualGraph& GetCurrentGraph() { return currentGraph_; }
    static bool HasGraph() { return !currentGraph_.id.empty(); }

private:
    static void RenderNodePalette();
    static void RenderGraphCanvas();
    static void RenderPropertyPanel();
    static void RenderMenuBar();
    static void RenderConnections();

    static void AddNode(const std::string& definitionId, float x, float y);
    static void RemoveNode(const std::string& nodeId);
    static void SelectNode(const std::string& nodeId);
    static void DeselectAll();

    static visualscripting::VisualGraph currentGraph_;
    static std::vector<EditorNode> editorNodes_;
    static std::vector<EditorConnection> editorConnections_;
    static std::string selectedNodeId_;
    static bool showNodePalette_;
    static bool showPropertyPanel_;
    static float canvasOffsetX_;
    static float canvasOffsetY_;
    static float canvasZoom_;
    static std::string currentFilePath_;
    static std::string statusMessage_;
    static bool isDraggingConnection_;
    static std::string dragFromNodeId_;
    static std::string dragFromPinId_;
    static float dragStartX_;
    static float dragStartY_;
    static float dragEndX_;
    static float dragEndY_;
    static int nextNodeId_;
};

} // namespace editor
} // namespace ge