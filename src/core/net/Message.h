#pragma once

// ================================================================
//  Message.h
//  Message types and serialization for networking.
// ================================================================

#include <vector>
#include <string>
#include <cstdint>
#include <memory>
#include <functional>
#include <chrono>
#include <type_traits>
#include <atomic>
#include <queue>
#include <mutex>

namespace ge {
namespace net {

enum class MessageType : uint16_t {
    // Internal / Handshake
    Connect = 1,
    ConnectAccept,
    ConnectReject,
    Disconnect,
    DisconnectAck,
    Ping,
    Pong,
    Welcome,
    
    // Game messages
    PlayerJoin = 100,
    PlayerLeave,
    PlayerState,
    PlayerInput,
    PlayerAction,
    ChatMessage,
    
    // Entity messages
    EntityCreate = 200,
    EntityUpdate,
    EntityDestroy,
    EntitySpawn,
    EntityDespawn,
    
    // World messages
    WorldState = 300,
    WorldTime,
    WorldEvent,
    
    // Custom
    Custom = 1000
};

struct MessageHeader {
    MessageType type = MessageType::Custom;
    uint16_t sequence = 0;
    uint8_t flags = 0;
    uint32_t senderId = 0;
    uint32_t size = 0;
};

enum class MessageFlag : uint8_t {
    None = 0,
    Reliable = 1 << 0,
    Ordered = 1 << 1,
    Urgent = 1 << 2,
    Fragmented = 1 << 3,
    Compressed = 1 << 4,
    Encrypted = 1 << 5
};

inline MessageFlag operator|(MessageFlag a, MessageFlag b) {
    return static_cast<MessageFlag>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
inline MessageFlag operator&(MessageFlag a, MessageFlag b) {
    return static_cast<MessageFlag>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

class Message {
public:
    Message();
    explicit Message(MessageType type);
    Message(MessageType type, const void* data, size_t size);
    ~Message();

    Message(const Message& other);
    Message& operator=(const Message& other);

    Message(Message&& other) noexcept;
    Message& operator=(Message&& other) noexcept;

    // Header access
    const MessageHeader& GetHeader() const { return header_; }
    MessageHeader& GetHeader() { return header_; }

    MessageType GetType() const { return header_.type; }
    void SetType(MessageType type) { header_.type = type; }

    uint16_t GetSequence() const { return header_.sequence; }
    void SetSequence(uint16_t seq) { header_.sequence = seq; }

    uint32_t GetSenderID() const { return header_.senderId; }
    void SetSenderID(uint32_t id) { header_.senderId = id; }

    MessageFlag GetFlags() const { return static_cast<MessageFlag>(header_.flags); }
    void SetFlags(MessageFlag flags) { header_.flags = static_cast<uint8_t>(flags); }

    // Payload access
    const uint8_t* GetData() const { return data_.data(); }
    uint8_t* GetData() { return data_.data(); }

    size_t GetSize() const { return data_.size(); }
    size_t GetCapacity() const { return data_.capacity(); }

    void SetData(const void* buffer, size_t size);
    void AppendData(const void* buffer, size_t size);

    void Clear();
    void Reserve(size_t capacity);

    // Serialization helpers
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
    void WriteString(const std::string& value);
    void WriteBytes(const void* data, size_t size);

    template<typename T>
    void Write(const T& value) {
        WriteBytes(&value, sizeof(T));
    }

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
    std::string ReadString();
    void ReadBytes(void* buffer, size_t size);

    template<typename T>
    T Read() {
        T value;
        ReadBytes(&value, sizeof(T));
        return value;
    }

    bool IsValid() const { return valid_; }
    void Invalidate() { valid_ = false; }

    size_t GetReadPosition() const { return readPosition_; }
    void SetReadPosition(size_t pos) { readPosition_ = pos; }

    static uint16_t NextSequence() { return nextSequence_++; }

private:
    MessageHeader header_;
    std::vector<uint8_t> data_;
    size_t readPosition_ = 0;
    bool valid_ = true;

    static std::atomic<uint16_t> nextSequence_;
};

using MessageHandler = std::function<void(uint32_t senderId, Message&)>;

class MessageQueue {
public:
    MessageQueue();
    ~MessageQueue();

    void Enqueue(Message&& msg);
    void Enqueue(const Message& msg);

    bool Dequeue(Message& msg);
    std::unique_ptr<Message> Dequeue();

    bool TryDequeue(Message& msg);
    std::unique_ptr<Message> TryDequeue();

    size_t Size() const;
    bool Empty() const;
    void Clear();

    void SetMaxSize(size_t maxSize);
    size_t GetMaxSize() const;

private:
    std::queue<Message> queue_;
    size_t maxSize_ = 256;
    std::mutex mutex_;
};

class MessageBuffer {
public:
    MessageBuffer();
    explicit MessageBuffer(size_t capacity);
    ~MessageBuffer();

    void Clear();
    void Reserve(size_t capacity);

    Message* BeginWrite(size_t size);
    void EndWrite(size_t size);

    const Message* BeginRead() const;
    void EndRead(size_t size);

    size_t Size() const { return size_; }
    size_t Capacity() const { return capacity_; }
    bool Empty() const { return size_ == 0; }

private:
    std::vector<uint8_t> buffer_;
    size_t capacity_ = 4096;
    size_t size_ = 0;
    size_t readPosition_ = 0;
};

class MessageFactory {
public:
    static Message CreatePing();
    static Message CreatePong();
    static Message CreateConnect(uint32_t playerId);
    static Message CreateConnectAccept(uint32_t playerId);
    static Message CreateDisconnect();
    static Message CreateWelcome(uint32_t playerId);
    static Message CreateChatMessage(uint32_t senderId, const std::string& message);
    static Message CreatePlayerState(uint32_t playerId, const void* stateData, size_t stateSize);
};

} // namespace net
} // namespace ge
