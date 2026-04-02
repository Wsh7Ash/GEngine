#pragma once

// ================================================================
//  Compression.h
//  Compression wrappers for asset streaming and packaging.
// ================================================================

#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <unordered_map>

namespace ge {
namespace assets {

enum class CompressionType : uint8_t {
    None = 0,
    LZ4 = 1,
    Zstd = 2,
    LZMA = 3
};

struct CompressionResult {
    bool success;
    std::vector<uint8_t> data;
    size_t originalSize;
    size_t compressedSize;
    float compressionRatio;
    std::string errorMessage;
    
    CompressionResult() : success(false), originalSize(0), compressedSize(0), compressionRatio(0.0f) {}
};

class Compression {
public:
    static CompressionResult Compress(const std::vector<uint8_t>& data, CompressionType type);
    static CompressionResult Decompress(const std::vector<uint8_t>& compressedData, size_t decompressedSize, CompressionType type);
    
    static size_t GetCompressedBufferSize(size_t originalSize, CompressionType type);
    static const char* GetCompressionName(CompressionType type);
    static bool IsSupported(CompressionType type);
    
    static float GetCompressionRatio(CompressionType type, size_t originalSize);
};

class CompressionStream {
public:
    CompressionStream(CompressionType type);
    ~CompressionStream();
    
    void Reset();
    
    bool Compress(const uint8_t* data, size_t size);
    bool Decompress(const uint8_t* data, size_t compressedSize, size_t decompressedSize);
    
    const std::vector<uint8_t>& GetData() const { return outputBuffer_; }
    size_t GetSize() const { return outputBuffer_.size(); }
    
    bool IsValid() const { return isValid_; }
    CompressionType GetType() const { return type_; }
    
private:
    CompressionType type_;
    std::vector<uint8_t> outputBuffer_;
    bool isValid_ = false;
};

class LZ4Compressor {
public:
    static std::vector<uint8_t> Compress(const uint8_t* data, size_t size);
    static std::vector<uint8_t> Decompress(const uint8_t* data, size_t compressedSize, size_t decompressedSize);
    
    static size_t GetMaxCompressedSize(size_t originalSize);
};

class ZstdCompressor {
public:
    static std::vector<uint8_t> Compress(const uint8_t* data, size_t size, int compressionLevel = 3);
    static std::vector<uint8_t> Decompress(const uint8_t* data, size_t compressedSize, size_t decompressedSize);
    
    static size_t GetMaxCompressedSize(size_t originalSize);
};

class CompressionManager {
public:
    static CompressionManager& Get();
    
    void RegisterCompressor(CompressionType type, std::function<CompressionResult(const std::vector<uint8_t>&)> compressor,
                          std::function<CompressionResult(const std::vector<uint8_t>&, size_t)> decompressor);
    
    CompressionResult Compress(const std::vector<uint8_t>& data, CompressionType type);
    CompressionResult Decompress(const std::vector<uint8_t>& compressedData, size_t decompressedSize, CompressionType type);
    
    bool IsTypeSupported(CompressionType type) const;
    
    std::vector<CompressionType> GetSupportedTypes() const;

private:
    CompressionManager() = default;
    ~CompressionManager() = default;
    
    struct CompressorFunctions {
        std::function<CompressionResult(const std::vector<uint8_t>&)> compress;
        std::function<CompressionResult(const std::vector<uint8_t>&, size_t)> decompress;
    };
    
    std::unordered_map<CompressionType, CompressorFunctions> compressors_;
};

} // namespace assets
} // namespace ge
