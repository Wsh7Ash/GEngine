#include "EmscriptenAssetLoader.h"
#include "../debug/log.h"
#include <cstring>

#ifdef __EMSCRIPTEN__
void EmscriptenAssetLoader::FetchSuccess(emscripten_fetch_t* fetch) {
    std::vector<uint8_t> data(fetch->numBytes);
    std::memcpy(data.data(), fetch->data, fetch->numBytes);

    auto& loader = EmscriptenAssetLoader::Get();
    auto it = loader.activeRequests_.find(fetch->url);
    if (it != loader.activeRequests_.end()) {
        if (it->second->onSuccess) {
            it->second->onSuccess(std::move(data));
        }
        loader.RemoveRequest(fetch->url);
    }

    emscripten_fetch_close(fetch);
}

void EmscriptenAssetLoader::FetchError(emscripten_fetch_t* fetch) {
    std::string error = "Fetch failed: ";
    error += fetch->status != 0 ? std::to_string(fetch->status) : "network error";

    auto& loader = EmscriptenAssetLoader::Get();
    auto it = loader.activeRequests_.find(fetch->url);
    if (it != loader.activeRequests_.end()) {
        if (it->second->onError) {
            it->second->onError(error);
        }
        loader.RemoveRequest(fetch->url);
    }

    emscripten_fetch_close(fetch);
}
#endif

namespace ge {
namespace assets {

EmscriptenAssetLoader& EmscriptenAssetLoader::Get() {
    static EmscriptenAssetLoader instance;
    return instance;
}

EmscriptenAssetLoader::EmscriptenAssetLoader() = default;

EmscriptenAssetLoader::~EmscriptenAssetLoader() {
    Shutdown();
}

void EmscriptenAssetLoader::Initialize() {
    if (initialized_) return;
    initialized_ = true;
    GE_LOG_INFO("EmscriptenAssetLoader initialized");
}

void EmscriptenAssetLoader::Shutdown() {
    CancelAllRequests();
    initialized_ = false;
}

void EmscriptenAssetLoader::FetchFile(const std::string& url,
                                       std::function<void(std::vector<uint8_t>)> onSuccess,
                                       std::function<void(const std::string&)> onError) {
#ifdef __EMSCRIPTEN__
    auto request = std::make_unique<EmscriptenFetchRequest>();
    request->url = url;
    request->onSuccess = std::move(onSuccess);
    request->onError = std::move(onError);

    activeRequests_[url] = std::move(request);

    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.onsuccess = FetchSuccess;
    attr.onerror = FetchError;
    attr.timeout = 30000;

    emscripten_fetch(&attr, url.c_str());
#else
    if (onError) {
        onError("EmscriptenAssetLoader: Not supported on this platform");
    }
#endif
}

void EmscriptenAssetLoader::FetchText(const std::string& url,
                                       std::function<void(const std::string&)> onSuccess,
                                       std::function<void(const std::string&)> onError) {
#ifdef __EMSCRIPTEN__
    FetchFile(url, [onSuccess](std::vector<uint8_t> data) {
        std::string text(data.begin(), data.end());
        if (onSuccess) onSuccess(text);
    }, std::move(onError));
#else
    (void)url;
    (void)onSuccess;
    if (onError) {
        onError("EmscriptenAssetLoader: Not supported on this platform");
    }
#endif
}

void EmscriptenAssetLoader::FetchBinary(const std::string& url,
                                         std::function<void(std::vector<uint8_t>)> onSuccess,
                                         std::function<void(const std::string&)> onError) {
    FetchFile(url, std::move(onSuccess), std::move(onError));
}

void EmscriptenAssetLoader::CancelRequest(const std::string& url) {
#ifdef __EMSCRIPTEN__
    auto it = activeRequests_.find(url);
    if (it != activeRequests_.end()) {
        emscripten_fetch_close(nullptr);
        RemoveRequest(url);
    }
#endif
}

void EmscriptenAssetLoader::CancelAllRequests() {
#ifdef __EMSCRIPTEN__
    for (auto& pair : activeRequests_) {
        emscripten_fetch_close(nullptr);
    }
#endif
    activeRequests_.clear();
}

size_t EmscriptenAssetLoader::GetActiveRequests() const {
    return activeRequests_.size();
}

size_t EmscriptenAssetLoader::GetPendingRequests() const {
    return activeRequests_.size();
}

bool EmscriptenAssetLoader::IsSupported() const {
#ifdef __EMSCRIPTEN__
    return true;
#else
    return false;
#endif
}

void EmscriptenAssetLoader::RemoveRequest(const std::string& url) {
    activeRequests_.erase(url);
}

EmscriptenFileSystem& EmscriptenFileSystem::Get() {
    static EmscriptenFileSystem instance;
    return instance;
}

void EmscriptenFileSystem::Initialize() {
    GE_LOG_INFO("EmscriptenFileSystem initialized");
}

void EmscriptenFileSystem::Shutdown() {
}

bool EmscriptenFileSystem::FileExists(const std::string& path) {
#ifdef __EMSCRIPTEN__
    return EM_ASM_INT({
        try {
            return FS.analyzePath(UTF8ToString($0)).exists ? 1 : 0;
        } catch (e) {
            return 0;
        }
    }, path.c_str()) != 0;
#else
    return false;
#endif
}

void EmscriptenFileSystem::ReadFileAsync(const std::string& path,
                                         std::function<void(std::vector<uint8_t>)> onSuccess,
                                         std::function<void(const std::string&)> onError) {
    EmscriptenAssetLoader::Get().FetchFile(path, std::move(onSuccess), std::move(onError));
}

void EmscriptenFileSystem::ReadTextAsync(const std::string& path,
                                         std::function<void(const std::string&)> onSuccess,
                                         std::function<void(const std::string&)> onError) {
    EmscriptenAssetLoader::Get().FetchText(path, std::move(onSuccess), std::move(onError));
}

void AsyncTextureLoader::LoadTextureAsync(const std::string& path, LoadCallback callback) {
    EmscriptenAssetLoader::Get().FetchFile(path, 
        [callback](std::vector<uint8_t> data) {
            DecodeImage(std::move(data), callback);
        },
        [callback](const std::string& error) {
            callback(false, error);
        });
}

void AsyncTextureLoader::LoadTextureFromMemoryAsync(std::vector<uint8_t> data, LoadCallback callback) {
    DecodeImage(std::move(data), callback);
}

void AsyncTextureLoader::DecodeImage(std::vector<uint8_t> data, LoadCallback callback) {
#ifdef __EMSCRIPTEN__
    if (data.empty()) {
        callback(false, "Empty image data");
        return;
    }

    EM_ASM({
        var img = new Image();
        var array = $0;
        var size = $1;
        
        var bytes = new Uint8Array(size);
        for (var i = 0; i < size; i++) {
            bytes[i] = HEAPU8[array + i];
        }
        
        var blob = new Blob([bytes], {type: 'image/png'});
        var url = URL.createObjectURL(blob);
        
        img.onload = function() {
            _ge_assets_AsyncTextureLoader_onImageLoaded(img.width, img.height);
            URL.revokeObjectURL(url);
        };
        img.onerror = function() {
            _ge_assets_AsyncTextureLoader_onImageError();
            URL.revokeObjectURL(url);
        };
        img.src = url;
    }, data.data(), data.size());
#else
    (void)data;
    (void)callback;
    callback(false, "AsyncTextureLoader: Not supported on this platform");
#endif
}

} // namespace assets
} // namespace ge