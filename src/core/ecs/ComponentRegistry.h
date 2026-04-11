#pragma once

// ================================================================
//  ComponentRegistry.h
//  Compile-time type identification for components.
//
//  Every unique component struct gets assigned a unique integer ID.
//  This ID is used to index into the world's storage arrays.
// ================================================================

#include <cstdint>
#include <atomic>
#include <cctype>
#include <string>
#include <string_view>
#include <typeinfo>
#include <unordered_map>

namespace ge {
namespace ecs
{

using ComponentTypeID = std::uint32_t;

static constexpr ComponentTypeID MAX_COMPONENTS = 128;
static constexpr ComponentTypeID INVALID_COMPONENT_ID = 0xFFFFFFFF;

namespace internal
{
    /// Global counter for component type IDs.
    inline ComponentTypeID NextComponentID()
    {
        static std::atomic<ComponentTypeID> s_counter{0};
        return s_counter.fetch_add(1);
    }

    inline std::string_view TrimComponentName(std::string_view value) {
        while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) {
            value.remove_prefix(1);
        }
        while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) {
            value.remove_suffix(1);
        }
        return value;
    }

    inline std::string RemoveLeadingTypeKeywords(std::string_view value) {
        std::string result(TrimComponentName(value));

        constexpr std::string_view kPrefixes[] = {
            "struct ",
            "class ",
            "enum ",
            "const "
        };

        bool removedPrefix = true;
        while (removedPrefix) {
            removedPrefix = false;
            for (const auto prefix : kPrefixes) {
                if (result.rfind(prefix, 0) == 0) {
                    result.erase(0, prefix.size());
                    removedPrefix = true;
                }
            }
        }

        return result;
    }

    inline std::string ExtractLeafTypeName(std::string_view value) {
        std::string sanitized = RemoveLeadingTypeKeywords(value);
        const size_t scopePos = sanitized.rfind("::");
        if (scopePos != std::string::npos) {
            sanitized.erase(0, scopePos + 2);
        }
        return sanitized;
    }

    inline std::string StripComponentSuffix(std::string_view value) {
        std::string leaf = ExtractLeafTypeName(value);
        constexpr std::string_view kSuffix = "Component";
        if (leaf.size() > kSuffix.size() &&
            leaf.compare(leaf.size() - kSuffix.size(), kSuffix.size(), kSuffix) == 0) {
            leaf.erase(leaf.size() - kSuffix.size());
        }
        return leaf;
    }

    inline std::string NormalizeComponentName(std::string_view value) {
        std::string normalized;
        normalized.reserve(value.size());

        for (const char ch : value) {
            const unsigned char uch = static_cast<unsigned char>(ch);
            if (std::isalnum(uch)) {
                normalized.push_back(static_cast<char>(std::tolower(uch)));
            }
        }

        return normalized;
    }

    inline std::unordered_map<std::string, ComponentTypeID>& GetTypeNameMap() {
        static std::unordered_map<std::string, ComponentTypeID> map;
        return map;
    }

    inline std::unordered_map<ComponentTypeID, std::string>& GetTypeIDNameMap() {
        static std::unordered_map<ComponentTypeID, std::string> map;
        return map;
    }

    inline void RegisterComponentAlias(std::string_view name, ComponentTypeID id) {
        const std::string normalized = NormalizeComponentName(name);
        if (!normalized.empty()) {
            GetTypeNameMap()[normalized] = id;
        }
    }

    inline void RegisterComponentName(std::string_view name, ComponentTypeID id) {
        const std::string canonicalName = RemoveLeadingTypeKeywords(name);
        auto& idNameMap = GetTypeIDNameMap();
        auto [it, inserted] = idNameMap.emplace(id, canonicalName);
        if (!inserted && it->second.empty()) {
            it->second = canonicalName;
        }

        RegisterComponentAlias(canonicalName, id);

        const std::string leafName = ExtractLeafTypeName(canonicalName);
        if (!leafName.empty()) {
            RegisterComponentAlias(leafName, id);
        }

        const std::string baseAlias = StripComponentSuffix(canonicalName);
        if (!baseAlias.empty()) {
            RegisterComponentAlias(baseAlias, id);
        }
    }

    inline ComponentTypeID GetComponentTypeIDByName(const char* name) {
        if (name == nullptr || name[0] == '\0') {
            return INVALID_COMPONENT_ID;
        }

        const std::string normalized = NormalizeComponentName(name);
        auto it = GetTypeNameMap().find(normalized);
        if (it != GetTypeNameMap().end()) {
            return it->second;
        }
        return INVALID_COMPONENT_ID;
    }

    inline const char* GetComponentNameByID(ComponentTypeID id) {
        const auto it = GetTypeIDNameMap().find(id);
        if (it != GetTypeIDNameMap().end()) {
            return it->second.c_str();
        }
        return nullptr;
    }

    template <typename T>
    inline std::string BuildComponentTypeName() {
#if defined(_MSC_VER)
        constexpr std::string_view signature = __FUNCSIG__;
        constexpr std::string_view beginMarker = "BuildComponentTypeName<";
        constexpr std::string_view endMarker = ">(void)";
        const size_t begin = signature.find(beginMarker);
        if (begin != std::string_view::npos) {
            const size_t typeStart = begin + beginMarker.size();
            const size_t end = signature.find(endMarker, typeStart);
            if (end != std::string_view::npos && end > typeStart) {
                return RemoveLeadingTypeKeywords(signature.substr(typeStart, end - typeStart));
            }
        }
#elif defined(__clang__) || defined(__GNUC__)
        constexpr std::string_view signature = __PRETTY_FUNCTION__;
        constexpr std::string_view beginMarker = "T = ";
        const size_t begin = signature.find(beginMarker);
        if (begin != std::string_view::npos) {
            const size_t typeStart = begin + beginMarker.size();
            const size_t end = signature.find_first_of(";]", typeStart);
            if (end != std::string_view::npos && end > typeStart) {
                return RemoveLeadingTypeKeywords(signature.substr(typeStart, end - typeStart));
            }
        }
#endif
        return RemoveLeadingTypeKeywords(typeid(T).name());
    }
}

/**
 * @brief Get the unique TypeID for a component type T.
 * 
 * Each call for the same T will return the same ID.
 * Each call for a different T will return a new unique ID.
 */
template <typename T>
inline const char* GetComponentTypeName() noexcept
{
    static const std::string s_typeName = internal::BuildComponentTypeName<T>();
    return s_typeName.c_str();
}

template <typename T>
inline ComponentTypeID GetComponentTypeID() noexcept
{
    static const ComponentTypeID s_typeID = []() {
        const ComponentTypeID id = internal::NextComponentID();
        internal::RegisterComponentName(GetComponentTypeName<T>(), id);
        return id;
    }();
    return s_typeID;
}

} // namespace ecs
} // namespace ge
