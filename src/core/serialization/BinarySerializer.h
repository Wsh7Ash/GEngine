#pragma once

// ================================================================
//  BinarySerializer.h
//  Binary serialization for fast I/O and networking.
// ================================================================

#include "Serializer.h"
#include <vector>
#include <cstring>
#include <cstdint>
#include <string>
#include <memory>

namespace ge {
namespace serialization {

class BinarySerializer : public Serializer {
public:
    BinarySerializer();
    ~BinarySerializer() override;
    
    void Serialize(ISerializable* object, const std::string& filepath) override;
    void Deserialize(ISerializable* object, const std::string& filepath) override;
    
    void SerializeToString(ISerializable* object, std::string& output) override;
    void DeserializeFromString(ISerializable* object, const std::string& input) override;
    
    void SerializeToMemory(ISerializable* object, std::vector<uint8_t>& output) override;
    void DeserializeFromMemory(ISerializable* object, const uint8_t* data, size_t size) override;
    
    SerializationFormat GetFormat() const override { return SerializationFormat::Binary; }
    
    void WriteBool(bool value);
    void WriteInt8(int8_t value);
    void WriteUInt8(uint8_t value);
    void WriteInt16(int16_t value);
    void WriteUInt16(uint16_t value);
    void WriteInt32(int32_t value);
    void WriteUInt32(uint32_t value);
    void WriteInt64(int64_t value);
    void WriteUInt64(uint64_t value);
    void WriteFloat(float value);
    void WriteDouble(double value);
    void WriteString(const std::string& value);
    void WriteBytes(const void* data, size_t size);
    
    bool ReadBool();
    int8_t ReadInt8();
    uint8_t ReadUInt8();
    int16_t ReadInt16();
    uint16_t ReadUInt16();
    int32_t ReadInt32();
    uint32_t ReadUInt32();
    int64_t ReadInt64();
    uint64_t ReadUInt64();
    float ReadFloat();
    double ReadDouble();
    std::string ReadString();
    void ReadBytes(void* data, size_t size);
    
    void SetVersion(uint32_t version) { version_ = version; }
    uint32_t GetVersion() const { return version_; }
    
    void SetSwapEndianness(bool swap) { swapEndianness_ = swap; }
    bool GetSwapEndianness() const { return swapEndianness_; }
    
    size_t GetPosition() const { return position_; }
    void SetPosition(size_t pos) { position_ = pos; }
    void Seek(size_t pos) { position_ = pos; }
    size_t GetDataSize() const { return data_.size(); }
    
    void Reset();
    void Clear();
    
    const std::vector<uint8_t>& GetData() const { return data_; }
    
private:
    template<typename T>
    void SwapBytes(T& value) {
        if (sizeof(T) == 2) {
            value = ((value & 0xFF) << 8) | ((value >> 8) & 0xFF);
        } else if (sizeof(T) == 4) {
            value = ((value & 0xFF) << 24) | ((value & 0xFF00) << 8) |
                    ((value >> 8) & 0xFF00) | ((value >> 24) & 0xFF);
        } else if (sizeof(T) == 8) {
            uint64_t v = value;
            value = ((v & 0xFF) << 56) | ((v & 0xFF00) << 40) |
                    ((v & 0xFF0000) << 24) | ((v & 0xFF000000) << 8) |
                    ((v >> 8) & 0xFF000000) | ((v >> 24) & 0xFF0000) |
                    ((v >> 40) & 0xFF00) | ((v >> 56) & 0xFF);
        }
    }
    
    std::vector<uint8_t> data_;
    size_t position_ = 0;
    uint32_t version_ = 1;
    bool swapEndianness_ = false;
    bool isReading_ = false;
};

class BinarySerializerFactory {
public:
    static std::unique_ptr<Serializer> Create();
};

} // namespace serialization
} // namespace ge
