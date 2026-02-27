#include "ContentBrowserPanel.h"
#include <imgui.h>

namespace ge{
    namespace editor{
        static const std::filesystem::path s_AssetsDirectory = "../assets";
        
        ContentBrowserPanel::ContentBrowserPanel() : cur_dir_(s_AssetsDirectory){

        }

        // TODO: Add file selection
        void ContentBrowserPanel::OnImGuiRender(){
            ImGui::Begin("Content Browser");
            if(cur_dir_ != std::filesystem::path(s_AssetsDirectory)){
                if(ImGui::Button("<-Back")){
                    cur_dir_ = cur_dir_.parent_path();
                }
            }

            for(auto& directoryEntity : std::filesystem::directory_iterator(cur_dir_)){
                const auto& path = directoryEntity.path();
                auto relativePath = std::filesystem::relative(path, s_AssetsDirectory);
                std::string filenameString = path.filename().string();
                if(directoryEntity.is_directory()){
                    if(ImGui::Button(filenameString.c_str())){
                        cur_dir_ /= path.filename();
                    }
                }else{
                    if(ImGui::Button(filenameString.c_str())){
                        // TODO: Open file
                    }
                }
            }

            ImGui::End();
        }
    }
}