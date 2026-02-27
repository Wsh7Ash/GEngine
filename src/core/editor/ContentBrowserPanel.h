#pragma once
#include <filesystem>

namespace ge{
    namespace editor{
        class ContentBrowserPanel{
            public:
                ContentBrowserPanel();
                void OnImGuiRender();

            private:
                std::filesystem::path cur_dir_;
        };
        
    }
}