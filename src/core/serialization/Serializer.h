#pragma once

// ================================================================
//  Serializer.h
//  Base serialization interface for engine objects.
// ================================================================

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <type_traits>

namespace ge {
namespace serialization {

enum class SerializationFormat {
    JSON,
    Binary,
    XML,
    YAML
};

enum class SerializationFlags {
    None = 0,
    PrettyPrint = 1 << 0,
    Compress = 1 << 1,
    IncludeMetadata = 1 << 2,
    ValidateSchema = 1 << 3
};

inline SerializationFlags operator|(SerializationFlags a, SerializationFlags b) {
    return static_cast<SerializationFlags>(static_cast<int>(a) | static_cast<int>(b));
}

inline SerializationFlags operator&(SerializationFlags a, SerializationFlags b) {
    return static_cast<SerializationFlags>(static_cast<int>(a) & static_cast<int>(b));
}

struct SerializationContext {
    int version = 1;
    SerializationFlags flags = SerializationFlags::None;
    SerializationFormat format = SerializationFormat::JSON;
    std::string currentPath;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
    
    void AddWarning(const std::string& warning) { warnings.push_back(warning); }
    void AddError(const std::string& error) { errors.push_back(error); }
    bool HasErrors() const { return !errors.empty(); }
};

class ISerializable {
public:
    virtual ~ISerializable() = default;
    virtual void Serialize(SerializationContext& ctx) const = 0;
    virtual void Deserialize(SerializationContext& ctx) = 0;
};

class Serializer {
public:
    virtual ~Serializer() = default;
    
    virtual void Serialize(ISerializable* object, const std::string& filepath) = 0;
    virtual void Deserialize(ISerializable* object, const std::string& filepath) = 0;
    
    virtual void SerializeToString(ISerializable* object, std::string& output) = 0;
    virtual void DeserializeFromString(ISerializable* object, const std::string& input) = 0;
    
    virtual void SerializeToMemory(ISerializable* object, std::vector<uint8_t>& output) = 0;
    virtual void DeserializeFromMemory(ISerializable* object, const uint8_t* data, size_t size) = 0;
    
    virtual SerializationFormat GetFormat() const = 0;
    virtual const SerializationContext& GetContext() const { return ctx_; }
    
protected:
    SerializationContext ctx_;
};

template<typename T>
std::enable_if_t<std::is_base_of_v<ISerializable, T>, void>
Serialize(T* object, const std::string& filepath, SerializationFormat format = SerializationFormat::JSON) {
    std::unique_ptr<Serializer> ser = CreateSerializer(format);
    ser->Serialize(object, filepath);
}

template<typename T>
std::enable_if_t<std::is_base_of_v<ISerializable, T>, void>
Deserialize(T* object, const std::string& filepath, SerializationFormat format = SerializationFormat::JSON) {
    std::unique_ptr<Serializer> ser = CreateSerializer(format);
    ser->Deserialize(object, filepath);
}

std::unique_ptr<Serializer> CreateSerializer(SerializationFormat format);

} // namespace serialization
} // namespace ge
