#include "ContentBrowserPanel.h"
#include <imgui.h>

namespace ge{
    namespace editor{
        static const std::filesystem::path s_AssetsDirectory = "path/to/assets";
        
        ContentBrowserPanel::ContentBrowserPanel() : cur_dir_(s_AssetsDirectory){

        }

        void ContentBrowserPanel::OnImGuiRender(){
            if(cur_dir_ != std::filesystem::path(s_AssetsDirectory)){
                if(ImGui::Button("<-Back")){
                    cur_dir_ = cur_dir_.parent_path();
                }
            }
        }
    }
}