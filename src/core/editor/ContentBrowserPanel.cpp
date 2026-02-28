#include "ContentBrowserPanel.h"
#include <imgui.h>

namespace ge{
    namespace editor{
        static const std::filesystem::path s_AssetsDirectory = "../assets";
        
        ContentBrowserPanel::ContentBrowserPanel() : cur_dir_(s_AssetsDirectory){

        }

        // TODO: Add file selection
        void ContentBrowserPanel::OnImGuiRender(){
            float padding = 16.0f;
            float thumbnailSize = 128.0f;
            float cellSize = thumbnailSize + padding;
            float panelWidth = ImGui::GetContentRegionAvail().x;
            int columnCount =  (int)(panelWidth / cellSize);
            
            columnCount = std::max(1, columnCount);
            ImGui::Columns(columnCount, 0, false);

            ImGui::Begin("Content Browser");
            if(cur_dir_ != std::filesystem::path(s_AssetsDirectory)){
                if(ImGui::Selectable("<-Back")){
                    cur_dir_ = cur_dir_.parent_path();
                }
            }

            for(auto& directoryEntry : std::filesystem::directory_iterator(cur_dir_)){
                for(auto& directoryEntity : std::filesystem::directory_iterator(cur_dir_)){
                    const auto& path = directoryEntity.path();
                    auto relativePath = std::filesystem::relative(path, s_AssetsDirectory);
                    std::string filenameString = path.filename().string();
                    if(directoryEntity.is_directory()){
                        if(ImGui::Selectable(filenameString.c_str())){
                            cur_dir_ /= path.filename();
                        }
                    }else{
                        if(ImGui::Selectable(filenameString.c_str())){
                            // TODO: Open file
                        }

                    }
                }
                ImGui::Button(filenameString.c_str(), ImVec2(thumbnailSize, thumbnailSize));
                ImGui::TextWrapped(filenameString.c_str());

                // Drag and drop
                if(ImGui::BeginDragDropSource()){
                    const wchar_t* itemPath = path.c_str();
                    ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", 
                                            itemPath, (wcslen(itemPath) + 1) * sizeof(wchar_t));
                    ImGui::TextUnformatted(filenameString.c_str());
                    ImGui::EndDragDropSource();
                }

                ImGui::NextColumn();
            }
            ImGui::Columns(1);

            ImGui::End();
        }
    }
}