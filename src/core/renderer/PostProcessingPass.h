#pragma once

#include "Framebuffer.h"
#include <memory>
#include <vector>

namespace ge {
namespace renderer {

    /**
     * @brief Interface for an individual post-processing pass.
     */
    class PostProcessingPass {
    public:
        virtual ~PostProcessingPass() = default;
        
        /**
         * @brief Execute the pass.
         * @param input The framebuffer containing the original scene or the output of the previous pass.
         * @param output The framebuffer to write the result to.
         */
        virtual void Execute(const std::shared_ptr<Framebuffer>& input, const std::shared_ptr<Framebuffer>& output) = 0;
    };

    /**
     * @brief Manages a chain of post-processing passes.
     */
    class PostProcessingStack {
    public:
        void AddPass(std::shared_ptr<PostProcessingPass> pass) {
            m_Passes.push_back(pass);
        }

        void Clear() {
            m_Passes.clear();
        }

        void Execute(std::shared_ptr<Framebuffer>& input, std::shared_ptr<Framebuffer>& intermediateA, std::shared_ptr<Framebuffer>& intermediateB) {
            if (m_Passes.empty()) return;

            std::shared_ptr<Framebuffer> currentInput = input;
            std::shared_ptr<Framebuffer> currentOutput = intermediateA;

            for (size_t i = 0; i < m_Passes.size(); ++i) {
                m_Passes[i]->Execute(currentInput, currentOutput);
                
                // Swap for next iteration
                currentInput = currentOutput;
                currentOutput = (currentInput == intermediateA) ? intermediateB : intermediateA;
            }

            // The final result is in currentInput
            input = currentInput;
        }

    private:
        std::vector<std::shared_ptr<PostProcessingPass>> m_Passes;
    };

} // namespace renderer
} // namespace ge
