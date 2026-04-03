#pragma once

// ================================================================
//  NetworkSerializer.h
//  Binary serialization for network messages.
// ================================================================

#include "Message.h"
#include <vector>
#include <string>
#include <cstring>
#include <type_traits>

namespace ge {
namespace net {

class NetworkSerializer {
public:
    NetworkSerializer();
    explicit NetworkSerializer(size_t capacity);
    ~NetworkSerializer();

    void Clear();
    void Reserve(size_t capacity);

    const uint8_t* GetData() const { return buffer_.data(); }
    uint8_t* GetData() { return buffer_.data(); }
    size_t GetSize() const { return position_; }
    size_t GetCapacity() const { return buffer_.capacity(); }

    // Primitive types
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
    void WriteBool(bool value);

    // Strings and bytes
    void WriteString(const std::string& value);
    void WriteStringFixed(const std::string& value, size_t maxLength);
    void WriteBytes(const void* data, size_t size);

    // Vectors
    template<typename T>
    void WriteVector(const std::vector<T>& vec) {
        WriteUInt32(static_cast<uint32_t>(vec.size()));
        for (const auto& item : vec) {
            Write(item);
        }
    }

    template<typename T, size_t N>
    void WriteArray(const T (&arr)[N]) {
        WriteUInt32(N);
        for (size_t i = 0; i < N; i++) {
            Write(arr[i]);
        }
    }

    // Raw write
    void Write(const void* data, size_t size);

    // Primitive reads
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
    bool ReadBool();

    // Strings
    std::string ReadString();
    std::string ReadStringFixed(size_t maxLength);
    void ReadBytes(void* data, size_t size);

    // Vectors
    template<typename T>
    std::vector<T> ReadVector() {
        uint32_t count = ReadUInt32();
        std::vector<T> vec;
        vec.reserve(count);
        for (uint32_t i = 0; i < count; i++) {
            vec.push_back(Read<T>());
        }
        return vec;
    }

    template<typename T>
    T Read() {
        T value;
        ReadBytes(&value, sizeof(T));
        return value;
    }

    // Rewind
    void SetPosition(size_t pos);
    size_t GetPosition() const { return position_; }
    void Skip(size_t bytes);

    bool HasData(size_t bytes) const;
    bool IsValid() const { return position_ <= buffer_.size(); }

private:
    std::vector<uint8_t> buffer_;
    size_t position_ = 0;
};

class NetworkDeserializer {
public:
    NetworkDeserializer();
    explicit NetworkDeserializer(const uint8_t* data, size_t size);
    ~NetworkDeserializer();

    void SetData(const uint8_t* data, size_t size);
    void Clear();

    const uint8_t* GetData() const { return data_; }
    size_t GetSize() const { return size_; }
    size_t GetPosition() const { return position_; }
    size_t Remaining() const { return size_ - position_; }

    // Primitive types
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
    bool ReadBool();

    // Strings
    std::string ReadString();
    std::string ReadStringFixed(size_t maxLength);
    void ReadBytes(void* data, size_t size);

    // Vectors
    template<typename T>
    std::vector<T> ReadVector() {
        uint32_t count = ReadUInt32();
        std::vector<T> vec;
        vec.reserve(count);
        for (uint32_t i = 0; i < count; i++) {
            vec.push_back(Read<T>());
        }
        return vec;
    }

    template<typename T>
    T Read() {
        T value;
        ReadBytes(&value, sizeof(T));
        return value;
    }

    void SetPosition(size_t pos);
    void Skip(size_t bytes);

    bool HasData(size_t bytes) const;
    bool IsValid() const { return position_ <= size_; }

private:
    const uint8_t* data_ = nullptr;
    size_t size_ = 0;
    size_t position_ = 0;
};

class AutoSerializer {
public:
    AutoSerializer() : serializer_(), deserializer_() {}
    ~AutoSerializer() = default;

    bool IsWriting() const { return writing_; }
    bool IsReading() const { return !writing_; }

    void SetWriting() { writing_ = true; deserializer_.Clear(); }
    void SetReading(const uint8_t* data, size_t size) {
        writing_ = false;
        deserializer_.SetData(data, size);
    }

    template<typename T>
    AutoSerializer& operator&(const T& value) {
        if (writing_) {
            serializer_.Write(value);
        } else {
            // Will need a const_cast for reading, or different approach
        }
        return *this;
    }

    NetworkSerializer& GetSerializer() { return serializer_; }
    NetworkDeserializer& GetDeserializer() { return deserializer_; }

    const uint8_t* GetData() const { return serializer_.GetData(); }
    size_t GetSize() const { return serializer_.GetSize(); }

private:
    NetworkSerializer serializer_;
    NetworkDeserializer deserializer_;
    bool writing_ = true;
};

} // namespace net
} // namespace ge
