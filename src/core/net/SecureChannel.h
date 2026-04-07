#pragma once

// ================================================================
//  SecureChannel.h
//  Encrypted transport channel for network connections.
// ================================================================

#include "CryptoTypes.h"
#include "Message.h"
#include "Socket.h"
#include "Connection.h"
#include <memory>
#include <queue>
#include <functional>

namespace ge {
namespace net {

class SecureChannel {
public:
    SecureChannel();
    ~SecureChannel();

    bool Initialize(const NetworkSecurityConfig& config);
    void Shutdown();

    bool SetPreSharedKey(const uint8_t* key, size_t keyLen);
    bool SetLocalKeyPair(const KeyPair& keyPair);
    bool SetRemotePublicKey(const uint8_t* publicKey, size_t len);

    bool IsEncrypted() const { return isEncrypted_; }
    bool IsAuthenticated() const { return isAuthenticated_; }
    bool IsEstablished() const { return handshakeComplete_; }

    CryptoResult Encrypt(const void* plaintext, size_t plainLen, 
                         std::vector<uint8_t>& outCiphertext, 
                         Nonce& outNonce);

    CryptoResult Decrypt(const void* ciphertext, size_t cipherLen, 
                         const Nonce& nonce,
                         std::vector<uint8_t>& outPlaintext);

    bool EncryptMessage(const Message& msg, std::vector<uint8_t>& outData);
    bool DecryptMessage(const std::vector<uint8_t>& inData, Message& outMsg);

    uint64_t GetSequenceNumber() const { return sendSequence_; }
    void SetReceiveCallback(std::function<void(const Message&)> callback);

    void ResetSequenceCounters();

    bool ProcessHandshakeMessage(const std::vector<uint8_t>& data, 
                                  std::vector<uint8_t>& response);

    bool IsHandshakeComplete() const { return handshakeComplete_; }

private:
    bool DeriveSessionKey();
    uint64_t GenerateSessionId();
    void GenerateNonce(Nonce& nonce);

    bool ProcessKeyExchangeResponse(const uint8_t* data, size_t len);
    bool ProcessKeyExchangeConfirm(const uint8_t* data, size_t len);

    NetworkSecurityConfig config_;
    bool isEncrypted_ = false;
    bool isAuthenticated_ = false;
    bool handshakeComplete_ = false;

    KeyPair localKeyPair_;
    PublicKey remotePublicKey_;
    SecretKey sessionKey_;
    Hash sessionHash_;

    uint64_t sendSequence_ = 0;
    uint64_t recvSequence_ = 0;
    uint32_t sessionId_ = 0;

    std::function<void(const Message&)> messageCallback_;

    std::queue<std::pair<Message, SendMode>> encryptedSendQueue_;
    std::queue<std::unique_ptr<Message>> encryptedRecvQueue_;
};

class SecureConnection {
public:
    SecureConnection();
    ~SecureConnection();

    void SetUnderlyingConnection(std::shared_ptr<Connection> conn);
    std::shared_ptr<Connection> GetUnderlyingConnection() const { return connection_; }

    bool InitializeSecurity(const NetworkSecurityConfig& config);
    void SetSecurityCredentials(const KeyPair& keyPair, const uint8_t* serverKey, size_t keyLen);

    void Connect(const SocketAddress& address) const;
    void Disconnect() const;

    void Send(const Message& msg, SendMode mode = SendMode::Reliable);
    std::unique_ptr<Message> Receive();

    ConnectionState GetState() const;
    bool IsSecure() const { return secureChannel_ && secureChannel_->IsEncrypted(); }

    void Update(float dt);

    float GetLatency() const;
    float GetPacketLoss() const;

    uint64_t GetBytesSent() const;
    uint64_t GetBytesReceived() const;

private:
    void ProcessSecureSend();
    void ProcessSecureReceive();

    std::shared_ptr<Connection> connection_;
    std::unique_ptr<SecureChannel> secureChannel_;
    bool securityInitialized_ = false;
};

class SecureServer {
public:
    SecureServer();
    ~SecureServer();

    bool Initialize(const NetworkSecurityConfig& config);
    void Shutdown();

    bool AddAuthorizedKey(const std::string& keyHex);
    bool RemoveAuthorizedKey(const std::string& keyHex);
    bool LoadAuthorizedKeysFromFile(const std::string& path);

    bool IsClientAuthorized(const uint8_t* publicKey, size_t len) const;

    std::shared_ptr<SecureChannel> CreateClientChannel();

    const NetworkSecurityConfig& GetConfig() const { return config_; }

private:
    NetworkSecurityConfig config_;
    std::vector<PublicKey> authorizedKeys_;
    KeyPair serverKeyPair_;
};

} // namespace net
} // namespace ge