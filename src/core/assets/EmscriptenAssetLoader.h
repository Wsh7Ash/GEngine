#pragma once

// ================================================================
//  EmscriptenAssetLoader.h
//  Async asset loading using Emscripten's Fetch API.
// ================================================================

/**
 * @file EmscriptenAssetLoader.h
 * @brief Async asset loading for WebAssembly targets.
 * 
 * Uses Emscripten's Fetch API for non-blocking file I/O.
 * Falls back to standard file I/O on native platforms.
 * 
 * @note Requires -s USE FETCH=1 linker flag for Emscripten builds.
 */

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>

#ifdef __EMSCRIPTEN__
#include <emscripten/fetch.h>
#endif

namespace ge {
namespace assets {

struct EmscriptenFetchRequest {
    std::string url;
    std::function<void(std::vector<uint8_t>)> onSuccess;
    std::function<void(const std::string&)> onError;
};

class EmscriptenAssetLoader {
public:
    static EmscriptenAssetLoader& Get();

    EmscriptenAssetLoader();
    ~EmscriptenAssetLoader();

    void Initialize();
    void Shutdown();

    void FetchFile(const std::string& url, 
                   std::function<void(std::vector<uint8_t>)> onSuccess,
                   std::function<void(const std::string&)> onError);

    void FetchText(const std::string& url,
                   std::function<void(const std::string&)> onSuccess,
                   std::function<void(const std::string&)> onError);

    void FetchBinary(const std::string& url,
                     std::function<void(std::vector<uint8_t>)> onSuccess,
                     std::function<void(const std::string&)> onError);

    void CancelRequest(const std::string& url);
    void CancelAllRequests();

    size_t GetActiveRequests() const;
    size_t GetPendingRequests() const;

    bool IsSupported() const;

private:
#ifdef __EMSCRIPTEN__
    static void FetchSuccess(emscripten_fetch_t* fetch);
    static void FetchError(emscripten_fetch_t* fetch);
#endif

    void RemoveRequest(const std::string& url);

    std::unordered_map<std::string, std::unique_ptr<EmscriptenFetchRequest>> activeRequests_;
    bool initialized_ = false;
};

class EmscriptenFileSystem {
public:
    static EmscriptenFileSystem& Get();

    void Initialize();
    void Shutdown();

    bool FileExists(const std::string& path);

    void ReadFileAsync(const std::string& path,
                       std::function<void(std::vector<uint8_t>)> onSuccess,
                       std::function<void(const std::string&)> onError);

    void ReadTextAsync(const std::string& path,
                       std::function<void(const std::string&)> onSuccess,
                       std::function<void(const std::string&)> onError);

    std::string GetMountPoint() const { return mountPoint_; }
    void SetMountPoint(const std::string& mountPoint) { mountPoint_ = mountPoint; }

private:
    std::string mountPoint_ = "/";
};

class AsyncTextureLoader {
public:
    using LoadCallback = std::function<void(bool success, const std::string& error)>;

    static void LoadTextureAsync(const std::string& path, LoadCallback callback);

    static void LoadTextureFromMemoryAsync(std::vector<uint8_t> data, 
                                            LoadCallback callback);

private:
    static void DecodeImage(std::vector<uint8_t> data, LoadCallback callback);
};

} // namespace assets
} // namespace ge