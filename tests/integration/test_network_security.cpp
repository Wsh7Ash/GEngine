#include "../catch_amalgamated.hpp"

#include "../../src/core/net/CryptoTypes.h"
#include "../../src/core/net/SecureChannel.h"
#include <cstring>

using namespace ge::net;

TEST_CASE("Crypto - KeyPair Generation", "[network][security]")
{
    KeyPair kp;
    kp.Generate();
    
    REQUIRE(kp.IsValid() == true);
    
    KeyPair kp2 = KeyPair::GenerateNew();
    REQUIRE(kp2.IsValid() == true);
    
    bool different = false;
    for (size_t i = 0; i < crypto_PUBLICKEY_BYTES; ++i) {
        if (kp.publicKey[i] != kp2.publicKey[i]) {
            different = true;
            break;
        }
    }
    REQUIRE(different == true);
}

TEST_CASE("Crypto - KeyPair Save Load", "[network][security]")
{
    KeyPair kp1 = KeyPair::GenerateNew();
    std::string hex = kp1.ToHex();
    
    KeyPair kp2 = KeyPair::FromHex(hex);
    
    for (size_t i = 0; i < crypto_PUBLICKEY_BYTES; ++i) {
        REQUIRE(kp1.publicKey[i] == kp2.publicKey[i]);
    }
    for (size_t i = 0; i < crypto_SECRETKEY_BYTES; ++i) {
        REQUIRE(kp1.secretKey[i] == kp2.secretKey[i]);
    }
}

TEST_CASE("Crypto - KeyPair Hex Conversion", "[network][security]")
{
    KeyPair kp = KeyPair::GenerateNew();
    std::string hex = kp.ToHex();
    
    REQUIRE(hex.length() == (crypto_PUBLICKEY_BYTES + crypto_SECRETKEY_BYTES) * 2);
    
    KeyPair loaded = KeyPair::FromHex(hex);
    REQUIRE(loaded.IsValid() == true);
}

TEST_CASE("Crypto - Invalid Hex", "[network][security]")
{
    KeyPair kp = KeyPair::FromHex("invalid");
    REQUIRE(kp.IsValid() == false);
}

TEST_CASE("Crypto - Security Level Enum", "[network][security]")
{
    REQUIRE(static_cast<int>(SecurityLevel::None) >= 0);
    REQUIRE(static_cast<int>(SecurityLevel::Lan) >= 0);
    REQUIRE(static_cast<int>(SecurityLevel::Standard) >= 0);
    REQUIRE(static_cast<int>(SecurityLevel::Paranoid) >= 0);
}

TEST_CASE("Crypto - Key Exchange Method Enum", "[network][security]")
{
    REQUIRE(static_cast<int>(KeyExchangeMethod::None) >= 0);
    REQUIRE(static_cast<int>(KeyExchangeMethod::PreSharedKey) >= 0);
    REQUIRE(static_cast<int>(KeyExchangeMethod::X25519) >= 0);
    REQUIRE(static_cast<int>(KeyExchangeMethod::ServerAssigned) >= 0);
}

TEST_CASE("Crypto - Cipher Type Enum", "[network][security]")
{
    REQUIRE(static_cast<int>(CipherType::None) >= 0);
    REQUIRE(static_cast<int>(CipherType::ChaCha20Poly1305) >= 0);
    REQUIRE(static_cast<int>(CipherType::AES256GCM) >= 0);
    REQUIRE(static_cast<int>(CipherType::XChaCha20Poly1305) >= 0);
}

TEST_CASE("Crypto - Network Security Config Default", "[network][security]")
{
    NetworkSecurityConfig config;
    
    REQUIRE(config.level == SecurityLevel::Standard);
    REQUIRE(config.keyExchange == KeyExchangeMethod::X25519);
    REQUIRE(config.cipher == CipherType::ChaCha20Poly1305);
    REQUIRE(config.requireAuthentication == true);
    REQUIRE(config.allowUnencryptedFallback == true);
}

TEST_CASE("Crypto - PSK Configuration", "[network][security]")
{
    NetworkSecurityConfig config;
    config.keyExchange = KeyExchangeMethod::PreSharedKey;
    
    std::vector<uint8_t> psk = {0x01, 0x02, 0x03, 0x04};
    config.psk = psk;
    
    REQUIRE(config.keyExchange == KeyExchangeMethod::PreSharedKey);
    REQUIRE(config.psk.size() == 4);
    REQUIRE(config.psk[0] == 0x01);
}

TEST_CASE("Crypto - SecureChannel Creation", "[network][security]")
{
    SecureChannel channel;
    REQUIRE(channel.IsEncrypted() == false);
    REQUIRE(channel.IsEstablished() == false);
}

TEST_CASE("Crypto - SecureChannel Initialize None", "[network][security]")
{
    SecureChannel channel;
    NetworkSecurityConfig config;
    config.level = SecurityLevel::None;
    config.keyExchange = KeyExchangeMethod::None;
    config.cipher = CipherType::None;
    
    bool result = channel.Initialize(config);
    REQUIRE(result == true);
    REQUIRE(channel.IsEncrypted() == false);
}

TEST_CASE("Crypto - EncryptedPacket Structure", "[network][security]")
{
    EncryptedPacket packet;
    packet.sequence = 12345;
    packet.sessionId = 67890;
    
    for (size_t i = 0; i < crypto_NONCE_BYTES; ++i) {
        packet.nonce[i] = static_cast<uint8_t>(i);
    }
    for (size_t i = 0; i < crypto_MAC_BYTES; ++i) {
        packet.mac[i] = static_cast<uint8_t>(i + 10);
    }
    
    packet.ciphertext = {0x01, 0x02, 0x03, 0x04};
    
    REQUIRE(packet.sequence == 12345);
    REQUIRE(packet.sessionId == 67890);
    REQUIRE(packet.nonce[0] == 0);
    REQUIRE(packet.mac[0] == 10);
    REQUIRE(packet.ciphertext.size() == 4);
}

TEST_CASE("Crypto - Session Key Rotation Interval", "[network][security]")
{
    NetworkSecurityConfig config;
    config.sessionKeyRotationInterval = 600000;
    
    REQUIRE(config.sessionKeyRotationInterval == 600000);
}

TEST_CASE("Crypto - Public Key Array Size", "[network][security]")
{
    PublicKey pk = {};
    REQUIRE(pk.size() == crypto_PUBLICKEY_BYTES);
    
    SecretKey sk = {};
    REQUIRE(sk.size() == crypto_SECRETKEY_BYTES);
    
    Nonce nonce = {};
    REQUIRE(nonce.size() == crypto_NONCE_BYTES);
    
    MAC mac = {};
    REQUIRE(mac.size() == crypto_MAC_BYTES);
}

TEST_CASE("Crypto - Initialize and Shutdown", "[network][security]")
{
    bool initResult = CryptoInitializer::Initialize();
    REQUIRE(initResult == true);
    REQUIRE(CryptoInitializer::IsInitialized() == true);
    
    CryptoInitializer::Shutdown();
    REQUIRE(CryptoInitializer::IsInitialized() == false);
}