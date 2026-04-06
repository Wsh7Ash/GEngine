#pragma once

// ================================================================
//  AssetCache.h
//  LRU cache with memory budget and GUID-based lookups.
// ================================================================

#include "Asset.h"
#include <cstdint>
#include <cstddef>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <memory>
#include <vector>
#include <string>
#include <functional>

namespace ge {
namespace assets {

struct CacheEntry {
    AssetHandle handle;
    std::vector<uint8_t> data;
    size_t size;
    std::string path;
    std::string sourceHash;
    uint64_t lastAccess;
    uint32_t accessCount;
    bool isPinned;
    
    CacheEntry() : size(0), lastAccess(0), accessCount(0), isPinned(false) {}
    CacheEntry(AssetHandle h, const std::vector<uint8_t>& d, const std::string& p);
    CacheEntry(AssetHandle h, std::vector<uint8_t>&& d, const std::string& p);
};

class AssetCache {
public:
    AssetCache();
    ~AssetCache();
    
    void SetMaxMemory(size_t maxBytes);
    size_t GetMaxMemory() const { return maxMemory_; }
    size_t GetUsedMemory() const { return usedMemory_; }
    
    bool Contains(AssetHandle handle) const;
    bool Contains(const std::string& path) const;
    
    const std::vector<uint8_t>* Get(AssetHandle handle);
    const std::vector<uint8_t>* Get(const std::string& path);
    
    void Put(AssetHandle handle, const std::vector<uint8_t>& data, const std::string& path, const std::string& hash = "");
    void Put(AssetHandle handle, std::vector<uint8_t>&& data, const std::string& path, const std::string& hash = "");
    
    void Pin(AssetHandle handle);
    void Unpin(AssetHandle handle);
    
    void Remove(AssetHandle handle);
    void Remove(const std::string& path);
    void Clear();
    
    void Update(uint64_t currentTime);
    
    size_t GetEntryCount() const { return entries_.size(); }
    
    float GetHitRate() const;
    void ResetStats();
    
    bool IsValid(AssetHandle handle) const;
    std::string GetPath(AssetHandle handle) const;
    std::string GetHash(AssetHandle handle) const;
    
    std::vector<AssetHandle> GetHandles() const;

private:
    void Evict(size_t bytesNeeded);
    void Touch(AssetHandle handle);
    void Touch(const std::string& path);
    
    CacheEntry* FindEntry(AssetHandle handle) const;
    CacheEntry* FindEntry(const std::string& path) const;
    
    struct Entry {
        std::unique_ptr<CacheEntry> entry;
        std::list<AssetHandle>::iterator lruIter;
    };
    
    std::unordered_map<AssetHandle, Entry> entries_;
    std::unordered_map<std::string, AssetHandle> pathToHandle_;
    std::list<AssetHandle> lruOrder_;
    
    size_t maxMemory_ = 512 * 1024 * 1024;
    size_t usedMemory_ = 0;
    
    uint64_t hits_ = 0;
    uint64_t misses_ = 0;
    
    mutable std::mutex mutex_;
    
    bool allowEviction_ = true;
};

class AssetCacheManager {
public:
    static AssetCacheManager& Get();
    
    AssetCache* GetCache(const std::string& name);
    AssetCache* GetDefaultCache();
    
    void CreateCache(const std::string& name, size_t maxMemory);
    void DestroyCache(const std::string& name);
    
    void Update();
    
    void SetGlobalCacheBudget(size_t bytes);
    
    using CacheInvalidationCallback = std::function<void(AssetHandle, const std::string&)>;
    void RegisterInvalidationCallback(CacheInvalidationCallback callback);
    
private:
    AssetCacheManager() = default;
    ~AssetCacheManager() = default;
    
    std::unordered_map<std::string, std::unique_ptr<AssetCache>> caches_;
    std::unique_ptr<AssetCache> defaultCache_;
    std::vector<CacheInvalidationCallback> invalidationCallbacks_;
};

} // namespace assets
} // namespace ge