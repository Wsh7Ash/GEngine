#pragma once

// ================================================================
//  CryptoTypes.h
//  Type definitions for cryptographic operations.
// ================================================================

#include <cstdint>
#include <vector>
#include <string>
#include <array>

namespace ge {
namespace net {

constexpr size_t crypto_SECRETKEY_BYTES = 32;
constexpr size_t crypto_PUBLICKEY_BYTES = 32;
constexpr size_t crypto_NONCE_BYTES = 24;
constexpr size_t crypto_MAC_BYTES = 16;
constexpr size_t crypto_HASH_BYTES = 32;
constexpr size_t crypto_SIG_BYTES = 64;

using SecretKey = std::array<uint8_t, crypto_SECRETKEY_BYTES>;
using PublicKey = std::array<uint8_t, crypto_PUBLICKEY_BYTES>;
using Nonce = std::array<uint8_t, crypto_NONCE_BYTES>;
using MAC = std::array<uint8_t, crypto_MAC_BYTES>;
using Hash = std::array<uint8_t, crypto_HASH_BYTES>;
using Signature = std::array<uint8_t, crypto_SIG_BYTES>;

enum class CryptoResult {
    Success = 0,
    InvalidKey,
    InvalidNonce,
    EncryptionFailed,
    DecryptionFailed,
    AuthenticationFailed,
    KeyGenerationFailed,
    NotInitialized
};

enum class KeyExchangeMethod {
    None,
    PreSharedKey,
    X25519,
    ServerAssigned
};

enum class CipherType {
    None,
    ChaCha20Poly1305,
    AES256GCM,
    XChaCha20Poly1305
};

enum class SecurityLevel {
    None,
    Lan,
    Standard,
    Paranoid
};

struct NetworkSecurityConfig {
    SecurityLevel level = SecurityLevel::Standard;
    KeyExchangeMethod keyExchange = KeyExchangeMethod::X25519;
    CipherType cipher = CipherType::ChaCha20Poly1305;
    
    std::vector<uint8_t> psk;
    std::string publicKeyPath;
    std::string privateKeyPath;
    std::vector<uint8_t> serverPublicKey;
    
    bool requireAuthentication = true;
    bool allowUnencryptedFallback = true;
    uint32_t sessionKeyRotationInterval = 300000;
};

struct EncryptedPacket {
    uint64_t sequence;
    uint32_t sessionId;
    uint8_t nonce[crypto_NONCE_BYTES];
    uint8_t mac[crypto_MAC_BYTES];
    std::vector<uint8_t> ciphertext;
};

struct KeyPair {
    PublicKey publicKey;
    SecretKey secretKey;
    
    bool IsValid() const;
    void Generate();
    
    static KeyPair GenerateNew();
    
    bool SaveToFile(const std::string& path) const;
    bool LoadFromFile(const std::string& path);
    
    std::string ToHex() const;
    static KeyPair FromHex(const std::string& hex);
};

class CryptoInitializer {
public:
    static bool Initialize();
    static bool IsInitialized();
    static void Shutdown();
    
private:
    static bool initialized_;
};

} // namespace net
} // namespace ge