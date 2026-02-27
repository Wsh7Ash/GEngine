#pragma once
#include <filesystem>

namespace ge{
    namespace editor{
        class ContentBrowserPanel{
            public:
                ContentBrowserPanel();
                void onImGuiRender();

            private:
                std::filesystem::path cur_dir_;
        };
        
    }
}