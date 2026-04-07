#include "CryptoTypes.h"
#include <algorithm>
#include <random>
#include <fstream>
#include <sstream>
#include <iomanip>

namespace ge {
namespace net {

bool CryptoInitializer::initialized_ = false;

bool CryptoInitializer::Initialize() {
    if (initialized_) return true;
    initialized_ = true;
    return true;
}

bool CryptoInitializer::IsInitialized() {
    return initialized_;
}

void CryptoInitializer::Shutdown() {
    initialized_ = false;
}

bool KeyPair::IsValid() const {
    bool allZero = true;
    for (size_t i = 0; i < crypto_PUBLICKEY_BYTES; ++i) {
        if (publicKey[i] != 0) {
            allZero = false;
            break;
        }
    }
    return !allZero;
}

void KeyPair::Generate() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFF);
    
    for (size_t i = 0; i < crypto_SECRETKEY_BYTES; ++i) {
        secretKey[i] = static_cast<uint8_t>(dist(gen) & 0xFF);
    }
    for (size_t i = 0; i < crypto_PUBLICKEY_BYTES; ++i) {
        publicKey[i] = static_cast<uint8_t>(dist(gen) & 0xFF);
    }
}

KeyPair KeyPair::GenerateNew() {
    KeyPair kp;
    kp.Generate();
    return kp;
}

bool KeyPair::SaveToFile(const std::string& path) const {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) return false;
    
    file.write(reinterpret_cast<const char*>(publicKey.data()), crypto_PUBLICKEY_BYTES);
    file.write(reinterpret_cast<const char*>(secretKey.data()), crypto_SECRETKEY_BYTES);
    return file.good();
}

bool KeyPair::LoadFromFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return false;
    
    file.read(reinterpret_cast<char*>(publicKey.data()), crypto_PUBLICKEY_BYTES);
    file.read(reinterpret_cast<char*>(secretKey.data()), crypto_SECRETKEY_BYTES);
    return file.good();
}

std::string KeyPair::ToHex() const {
    std::ostringstream ss;
    ss << std::hex << std::setfill('0');
    for (size_t i = 0; i < crypto_PUBLICKEY_BYTES; ++i) {
        ss << std::setw(2) << static_cast<int>(publicKey[i]);
    }
    for (size_t i = 0; i < crypto_SECRETKEY_BYTES; ++i) {
        ss << std::setw(2) << static_cast<int>(secretKey[i]);
    }
    return ss.str();
}

KeyPair KeyPair::FromHex(const std::string& hex) {
    KeyPair kp;
    if (hex.size() != (crypto_PUBLICKEY_BYTES + crypto_SECRETKEY_BYTES) * 2) {
        return kp;
    }
    
    for (size_t i = 0; i < crypto_PUBLICKEY_BYTES; ++i) {
        std::string byteStr = hex.substr(i * 2, 2);
        kp.publicKey[i] = static_cast<uint8_t>(std::stoi(byteStr, nullptr, 16));
    }
    for (size_t i = 0; i < crypto_SECRETKEY_BYTES; ++i) {
        std::string byteStr = hex.substr((crypto_PUBLICKEY_BYTES + i) * 2, 2);
        kp.secretKey[i] = static_cast<uint8_t>(std::stoi(byteStr, nullptr, 16));
    }
    
    return kp;
}

} // namespace net
} // namespace ge