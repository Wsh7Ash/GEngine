#pragma once

#include "Command.h"
#include <vector>
#include <memory>

namespace ge {
namespace cmd {

    /**
     * @brief Manages the undo/redo stack.
     */
    class CommandHistory {
    public:
        static void PushCommand(std::unique_ptr<ICommand> command);
        static void Undo();
        static void Redo();
        static void Clear();

    private:
        static std::vector<std::unique_ptr<ICommand>> s_Commands;
        static int s_CommandIndex;
    };

} // namespace cmd
} // namespace ge
