#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

namespace ge {
namespace renderer {

    /**
     * @brief Interface for Cubemap textures (6 faces).
     */
    class Cubemap
    {
    public:
        virtual ~Cubemap() = default;

        virtual void Bind(uint32_t slot = 0) const = 0;
        virtual void Unbind() const = 0;
        virtual uint32_t GetID() const = 0;

        /**
         * @brief Create a cubemap from 6 image files.
         * Order: Right, Left, Top, Bottom, Front, Back.
         */
        static std::shared_ptr<Cubemap> Create(const std::vector<std::string>& faces);

        /**
         * @brief Create an empty cubemap of a certain size (e.g., for offscreen rendering).
         */
        static std::shared_ptr<Cubemap> Create(uint32_t size, bool hdr = false);
    };

} // namespace renderer
} // namespace ge
