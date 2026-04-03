#include "NetworkSerializer.h"
#include <cstring>

namespace ge {
namespace net {

NetworkSerializer::NetworkSerializer() {
    buffer_.reserve(1024);
}

NetworkSerializer::NetworkSerializer(size_t capacity) {
    buffer_.reserve(capacity);
}

NetworkSerializer::~NetworkSerializer() {
    buffer_.clear();
}

void NetworkSerializer::Clear() {
    position_ = 0;
    buffer_.clear();
}

void NetworkSerializer::Reserve(size_t capacity) {
    buffer_.reserve(capacity);
}

void NetworkSerializer::WriteInt8(int8_t value) {
    WriteBytes(&value, sizeof(int8_t));
}

void NetworkSerializer::WriteUInt8(uint8_t value) {
    WriteBytes(&value, sizeof(uint8_t));
}

void NetworkSerializer::WriteInt16(int16_t value) {
    WriteBytes(&value, sizeof(int16_t));
}

void NetworkSerializer::WriteUInt16(uint16_t value) {
    WriteBytes(&value, sizeof(uint16_t));
}

void NetworkSerializer::WriteInt32(int32_t value) {
    WriteBytes(&value, sizeof(int32_t));
}

void NetworkSerializer::WriteUInt32(uint32_t value) {
    WriteBytes(&value, sizeof(uint32_t));
}

void NetworkSerializer::WriteInt64(int64_t value) {
    WriteBytes(&value, sizeof(int64_t));
}

void NetworkSerializer::WriteUInt64(uint64_t value) {
    WriteBytes(&value, sizeof(uint64_t));
}

void NetworkSerializer::WriteFloat(float value) {
    WriteBytes(&value, sizeof(float));
}

void NetworkSerializer::WriteDouble(double value) {
    WriteBytes(&value, sizeof(double));
}

void NetworkSerializer::WriteBool(bool value) {
    WriteBytes(&value, sizeof(bool));
}

void NetworkSerializer::WriteString(const std::string& value) {
    WriteUInt32(static_cast<uint32_t>(value.size()));
    WriteBytes(value.data(), value.size());
}

void NetworkSerializer::WriteStringFixed(const std::string& value, size_t maxLength) {
    size_t length = std::min(value.size(), maxLength);
    WriteUInt32(static_cast<uint32_t>(length));
    WriteBytes(value.data(), length);
    if (length < maxLength) {
        uint8_t padding = 0;
        WriteBytes(&padding, maxLength - length);
    }
}

void NetworkSerializer::WriteBytes(const void* data, size_t size) {
    if (!data || size == 0) return;
    
    if (position_ + size > buffer_.capacity()) {
        buffer_.reserve(std::max(buffer_.capacity() * 2, position_ + size + 256));
    }
    
    if (position_ + size > buffer_.size()) {
        buffer_.resize(position_ + size);
    }
    
    std::memcpy(buffer_.data() + position_, data, size);
    position_ += size;
}

void NetworkSerializer::Write(const void* data, size_t size) {
    WriteBytes(data, size);
}

int8_t NetworkDeserializer::ReadInt8() {
    int8_t value;
    ReadBytes(&value, sizeof(int8_t));
    return value;
}

uint8_t NetworkDeserializer::ReadUInt8() {
    uint8_t value;
    ReadBytes(&value, sizeof(uint8_t));
    return value;
}

int16_t NetworkDeserializer::ReadInt16() {
    int16_t value;
    ReadBytes(&value, sizeof(int16_t));
    return value;
}

uint16_t NetworkDeserializer::ReadUInt16() {
    uint16_t value;
    ReadBytes(&value, sizeof(uint16_t));
    return value;
}

int32_t NetworkDeserializer::ReadInt32() {
    int32_t value;
    ReadBytes(&value, sizeof(int32_t));
    return value;
}

uint32_t NetworkDeserializer::ReadUInt32() {
    uint32_t value;
    ReadBytes(&value, sizeof(uint32_t));
    return value;
}

int64_t NetworkDeserializer::ReadInt64() {
    int64_t value;
    ReadBytes(&value, sizeof(int64_t));
    return value;
}

uint64_t NetworkDeserializer::ReadUInt64() {
    uint64_t value;
    ReadBytes(&value, sizeof(uint64_t));
    return value;
}

float NetworkDeserializer::ReadFloat() {
    float value;
    ReadBytes(&value, sizeof(float));
    return value;
}

double NetworkDeserializer::ReadDouble() {
    double value;
    ReadBytes(&value, sizeof(double));
    return value;
}

bool NetworkDeserializer::ReadBool() {
    bool value;
    ReadBytes(&value, sizeof(bool));
    return value;
}

std::string NetworkDeserializer::ReadString() {
    uint32_t length = ReadUInt32();
    if (length == 0) return "";
    
    if (!HasData(length)) return "";
    
    std::string value(reinterpret_cast<const char*>(data_ + position_), length);
    position_ += length;
    return value;
}

std::string NetworkDeserializer::ReadStringFixed(size_t maxLength) {
    uint32_t length = ReadUInt32();
    size_t actualLength = std::min(static_cast<size_t>(length), maxLength);
    
    if (!HasData(actualLength)) return "";
    
    std::string value(reinterpret_cast<const char*>(data_ + position_), actualLength);
    position_ += maxLength;
    return value;
}

void NetworkDeserializer::ReadBytes(void* data, size_t size) {
    if (!data || size == 0) return;
    if (!HasData(size)) return;
    
    std::memcpy(data, data_ + position_, size);
    position_ += size;
}

void NetworkDeserializer::SetData(const uint8_t* data, size_t size) {
    data_ = data;
    size_ = size;
    position_ = 0;
}

void NetworkDeserializer::Clear() {
    data_ = nullptr;
    size_ = 0;
    position_ = 0;
}

void NetworkDeserializer::SetPosition(size_t pos) {
    position_ = std::min(pos, size_);
}

void NetworkDeserializer::Skip(size_t bytes) {
    position_ = std::min(position_ + bytes, size_);
}

bool NetworkDeserializer::HasData(size_t bytes) const {
    return position_ + bytes <= size_;
}

NetworkDeserializer::NetworkDeserializer() : data_(nullptr), size_(0), position_(0) {}

NetworkDeserializer::NetworkDeserializer(const uint8_t* data, size_t size) 
    : data_(data), size_(size), position_(0) {}

NetworkDeserializer::~NetworkDeserializer() {
    data_ = nullptr;
    size_ = 0;
    position_ = 0;
}

void NetworkSerializer::SetPosition(size_t pos) {
    position_ = std::min(pos, buffer_.size());
}

void NetworkSerializer::Skip(size_t bytes) {
    position_ = std::min(position_ + bytes, buffer_.size());
}

bool NetworkSerializer::HasData(size_t bytes) const {
    return position_ + bytes <= buffer_.size();
}

} // namespace net
} // namespace ge
