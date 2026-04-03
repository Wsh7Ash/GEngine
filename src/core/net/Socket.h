#pragma once

// ================================================================
//  Socket.h
//  Minimal platform-abstracted socket layer.
// ================================================================

#include <string>
#include <vector>
#include <cstdint>
#include <functional>
#include <memory>

namespace ge {
namespace net {

enum class SocketResult {
    Success = 0,
    WouldBlock,
    ConnectionRefused,
    ConnectionReset,
    HostUnreachable,
    NetworkUnreachable,
    InvalidSocket,
    InvalidParameter,
    NotConnected,
    UnknownError
};

enum class SocketType {
    TCP,
    UDP
};

enum class SocketState {
    Closed,
    Listening,
    Connected,
    Bound
};

struct SocketAddress {
    std::string address;
    uint16_t port = 0;
    
    SocketAddress() = default;
    SocketAddress(const std::string& addr, uint16_t p);
    
    std::string ToString() const;
    bool IsValid() const;
    
    static SocketAddress Any(uint16_t port = 0);
    static SocketAddress Localhost(uint16_t port = 0);
    static SocketAddress Broadcast(uint16_t port);
};

class Socket {
public:
    Socket();
    explicit Socket(SocketType type);
    ~Socket();
    
    Socket(Socket&& other) noexcept;
    Socket& operator=(Socket&& other) noexcept;
    
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    
    SocketResult Bind(const SocketAddress& address);
    SocketResult Listen(int backlog = 5);
    SocketResult Connect(const SocketAddress& address);
    void Close();
    
    Socket Accept(SocketAddress* clientAddress = nullptr);
    int AcceptHandle(SocketAddress* clientAddress = nullptr);
    
    SocketResult Send(const void* data, size_t size, size_t* sentBytes = nullptr);
    SocketResult Receive(void* buffer, size_t size, size_t* receivedBytes = nullptr);
    
    SocketResult SendTo(const void* data, size_t size, const SocketAddress& to, size_t* sentBytes = nullptr);
    SocketResult ReceiveFrom(void* buffer, size_t size, SocketAddress& from, size_t* receivedBytes = nullptr);
    
    SocketResult SetBlocking(bool blocking);
    SocketResult SetReuseAddress(bool reuse);
    SocketResult SetBroadcast(bool broadcast);
    SocketResult SetNoDelay(bool nodelay);
    
    SocketResult SetReceiveTimeout(int timeoutMs);
    SocketResult SetSendTimeout(int timeoutMs);
    
    bool IsValid() const;
    bool IsBlocking() const;
    SocketState GetState() const { return state_; }
    SocketType GetType() const { return type_; }
    
    Socket GetSocket() const { return socket_; }
    
    static bool Initialize();
    static void Shutdown();
    
    static std::string GetLastError();
    static std::string GetErrorString(SocketResult result);
    
private:
    Socket(SocketType type, int sock);
    int socket_ = -1;
    SocketType type_ = SocketType::TCP;
    SocketState state_ = SocketState::Closed;
    bool blocking_ = true;
};

class UDPSocket : public Socket {
public:
    UDPSocket();
    ~UDPSocket() = default;
    
    SocketResult Send(const void* data, size_t size, const SocketAddress& to);
    SocketResult Receive(void* buffer, size_t size, SocketAddress& from);
    
    SocketResult EnableBroadcast();
    SocketResult JoinMulticastGroup(const std::string& group);
    SocketResult LeaveMulticastGroup(const std::string& group);
};

class TCPSocket : public Socket {
public:
    TCPSocket();
    ~TCPSocket() = default;
    
    SocketResult Connect(const SocketAddress& address);
    SocketResult Listen(uint16_t port, int backlog = 5);
    
    std::shared_ptr<TCPSocket> Accept();
    
    SocketResult EnableNagle(bool enable);
};

class DNS {
public:
    static bool Resolve(const std::string& hostname, std::vector<SocketAddress>& results);
    static SocketAddress ResolveFirst(const std::string& hostname, uint16_t port = 0);
};

} // namespace net
} // namespace ge
