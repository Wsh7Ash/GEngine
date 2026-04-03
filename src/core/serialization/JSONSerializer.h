#pragma once

// ================================================================
//  JSONSerializer.h
//  JSON-based serialization implementation.
// ================================================================

#include "Serializer.h"
#include "EntitySerializer.h"
#include <string>
#include <vector>
#include <cstdint>
#include <fstream>
#include <memory>

namespace ge {
namespace serialization {

class JSONSerializer : public Serializer {
public:
    JSONSerializer();
    ~JSONSerializer() override;
    
    void Serialize(ISerializable* object, const std::string& filepath) override;
    void Deserialize(ISerializable* object, const std::string& filepath) override;
    
    void SerializeToString(ISerializable* object, std::string& output) override;
    void DeserializeFromString(ISerializable* object, const std::string& input) override;
    
    void SerializeToMemory(ISerializable* object, std::vector<uint8_t>& output) override;
    void DeserializeFromMemory(ISerializable* object, const uint8_t* data, size_t size) override;
    
    SerializationFormat GetFormat() const override { return SerializationFormat::JSON; }
    
    void SetPrettyPrint(bool enabled) { prettyPrint_ = enabled; }
    bool GetPrettyPrint() const { return prettyPrint_; }
    
    void WriteStartObject(const std::string& key);
    void WriteEndObject();
    void WriteStartArray(const std::string& key);
    void WriteEndArray();
    void WriteKey(const std::string& key);
    void WriteValue(const std::string& value);
    void WriteValue(int value);
    void WriteValue(float value);
    void WriteValue(double value);
    void WriteValue(bool value);
    void WriteValue(uint64_t value);
    void WriteNull();
    
    void ReadStartObject();
    void ReadEndObject();
    void ReadStartArray();
    void ReadEndArray();
    bool ReadKey(std::string& key);
    std::string ReadValue();
    int ReadIntValue();
    float ReadFloatValue();
    double ReadDoubleValue();
    bool ReadBoolValue();
    uint64_t ReadUInt64Value();
    bool IsNull();
    bool HasMore();
    
    bool IsReading() const { return isReading_; }
    bool IsWriting() const { return !isReading_; }
    
private:
    void EnsureNotReading() { if (isReading_) ctx_.AddError("Cannot write while reading"); }
    void EnsureReading() { if (!isReading_) ctx_.AddError("Cannot read while writing"); }
    
    std::string indentation_;
    bool prettyPrint_ = true;
    bool isReading_ = false;
    std::string currentKey_;
    std::string jsonContent_;
    size_t parsePosition_ = 0;
};

class JSONSerializerFactory {
public:
    static std::unique_ptr<Serializer> Create();
};

} // namespace serialization
} // namespace ge
