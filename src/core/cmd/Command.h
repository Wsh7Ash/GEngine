#pragma once

namespace ge {
namespace cmd {

    /**
     * @brief Interface for any undoable action in the editor.
     */
    class ICommand {
    public:
        virtual ~ICommand() = default;

        virtual void Execute() = 0;
        virtual void Undo() = 0;

        /**
         * @brief Attempt to merge this command with another.
         * Useful for collapsing multiple transform moves into one.
         */
        virtual bool MergeWith(ICommand* other) { return false; }
    };

} // namespace cmd
} // namespace ge
