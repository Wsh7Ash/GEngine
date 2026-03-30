#pragma once

#include <memory>
#include <string>
#include <imgui.h>
#include "../material/MaterialGraph.h"

namespace ge {
namespace editor {

class MaterialEditorPanel {
public:
    MaterialEditorPanel();
    void OnImGuiRender();
    
    void NewGraph();
    void OpenGraph(const std::string& filepath);
    void SaveGraph(const std::string& filepath);
    
    bool HasGraph() const { return graph_ != nullptr && !graph_->IsEmpty(); }
    
private:
    void DrawNodePalette();
    void DrawGraphCanvas();
    void DrawNodeProperties();
    void DrawGeneratedCode();
    
    void HandleDragDrop();
    bool IsPinHovered(int nodeId, int pinId, const ImVec2& mousePos);
    void BeginConnection(int nodeId, int pinId);
    void EndConnection(int nodeId, int pinId);
    
    std::shared_ptr<material::MaterialGraph> graph_;
    std::string currentFilepath_;
    
    int selectedNodeId_ = -1;
    ImVec2 contextMenuPos_;
    bool showContextMenu_ = false;
    
    int dragNodeId_ = -1;
    ImVec2 dragOffset_;
    
    int connectionStartNodeId_ = -1;
    int connectionStartPinId_ = -1;
    ImVec2 connectionStartPos_;
    bool isDraggingConnection_ = false;
    
    bool showCodePreview_ = false;
    std::string generatedCode_;
};

}
}
