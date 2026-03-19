#pragma once

#include <cstdint>
#include <cstddef>

namespace ge {

    class UUID
    {
    public:
        UUID();
        UUID(uint64_t uuid);
        UUID(const UUID&) = default;

        operator uint64_t() const { return uuid_; }
    private:
        uint64_t uuid_;
    };

} // namespace ge

namespace std {
    template <typename T> struct hash;

    template<>
    struct hash<ge::UUID>
    {
        std::size_t operator()(const ge::UUID& uuid) const
        {
            return (uint64_t)uuid;
        }
    };
}
