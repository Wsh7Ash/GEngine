#include "CommandHistory.h"

namespace ge {
namespace cmd {

    std::vector<std::unique_ptr<ICommand>> CommandHistory::s_Commands;
    int CommandHistory::s_CommandIndex = -1;

    void CommandHistory::PushCommand(std::unique_ptr<ICommand> command) {
        // If we are in the middle of the history, clear the redo future
        if (s_CommandIndex < (int)s_Commands.size() - 1) {
            s_Commands.erase(s_Commands.begin() + s_CommandIndex + 1, s_Commands.end());
        }

        // Try to merge with the previous command
        if (s_CommandIndex >= 0 && s_Commands[s_CommandIndex]->MergeWith(command.get())) {
            // Merged successfully, do not add to history
            return;
        }

        s_Commands.push_back(std::move(command));
        s_CommandIndex++;

        // Limit history size (optional, e.g., 100)
        if (s_Commands.size() > 100) {
            s_Commands.erase(s_Commands.begin());
            s_CommandIndex--;
        }
    }

    void CommandHistory::Undo() {
        if (s_CommandIndex >= 0) {
            s_Commands[s_CommandIndex]->Undo();
            s_CommandIndex--;
        }
    }

    void CommandHistory::Redo() {
        if (s_CommandIndex < (int)s_Commands.size() - 1) {
            s_CommandIndex++;
            s_Commands[s_CommandIndex]->Execute();
        }
    }

    void CommandHistory::Clear() {
        s_Commands.clear();
        s_CommandIndex = -1;
    }

} // namespace cmd
} // namespace ge
