#pragma once

// ================================================================
//  AssetCache.h
//  LRU cache with memory budget for asset data.
// ================================================================

#include <cstdint>
#include <cstddef>
#include <list>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <vector>
#include <string>

namespace ge {
namespace assets {

struct CacheEntry {
    std::vector<uint8_t> data;
    size_t size;
    std::string path;
    uint64_t lastAccess;
    uint32_t accessCount;
    bool isPinned;
    
    CacheEntry() : size(0), lastAccess(0), accessCount(0), isPinned(false) {}
    CacheEntry(const std::vector<uint8_t>& d, const std::string& p);
};

class AssetCache {
public:
    AssetCache();
    ~AssetCache();
    
    void SetMaxMemory(size_t maxBytes);
    size_t GetMaxMemory() const { return maxMemory_; }
    size_t GetUsedMemory() const { return usedMemory_; }
    
    bool Contains(const std::string& path) const;
    
    const std::vector<uint8_t>* Get(const std::string& path);
    
    void Put(const std::string& path, const std::vector<uint8_t>& data);
    void Put(const std::string& path, std::vector<uint8_t>&& data);
    
    void Pin(const std::string& path);
    void Unpin(const std::string& path);
    
    void Remove(const std::string& path);
    void Clear();
    
    void Update(uint64_t currentTime);
    
    size_t GetEntryCount() const { return entries_.size(); }
    
    float GetHitRate() const;
    void ResetStats();

private:
    void Evict(size_t bytesNeeded);
    void Touch(const std::string& path);
    
    CacheEntry* FindEntry(const std::string& path) const;
    
    std::unordered_map<std::string, std::unique_ptr<CacheEntry>> entries_;
    std::list<std::string> lruOrder_;
    
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
    
private:
    AssetCacheManager() = default;
    ~AssetCacheManager() = default;
    
    std::unordered_map<std::string, std::unique_ptr<AssetCache>> caches_;
    std::unique_ptr<AssetCache> defaultCache_;
};

} // namespace assets
} // namespace ge
