#pragma once

#include <string>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <atomic>
#include <chrono>

#include "lru_cache.h"
#include "memory_allocator.h"
#include "object_pool.h"

namespace cache {

class Cache {
public:
    explicit Cache(size_t max_capacity = 1024 * 1024 * 1024); // 1GB default
    ~Cache() = default;

    // Non-copyable, non-movable
    Cache(const Cache&) = delete;
    Cache& operator=(const Cache&) = delete;
    Cache(Cache&&) = delete;
    Cache& operator=(Cache&&) = delete;

    // Core operations
    bool set(const std::string& key, const std::string& value);
    std::string get(const std::string& key);
    bool remove(const std::string& key);
    void clear();

    // Statistics
    size_t size() const;
    size_t capacity() const;
    double hit_ratio() const;
    size_t hits() const;
    size_t misses() const;

    // Memory management
    size_t memory_usage() const;
    void set_max_capacity(size_t capacity);

public:
    struct CacheEntry {
        std::string key;
        std::string value;
        std::chrono::steady_clock::time_point timestamp;
        size_t access_count;
        
        CacheEntry() = default;
        CacheEntry(const std::string& k, const std::string& v)
            : key(k), value(v), timestamp(std::chrono::steady_clock::now()), access_count(0) {}
    };

private:

    mutable std::shared_mutex mutex_;
    LRUCache<std::string, CacheEntry> lru_cache_;
    std::unique_ptr<MemoryAllocator> allocator_;
    std::unique_ptr<ObjectPool<CacheEntry>> entry_pool_;
    
    // Statistics
    mutable std::atomic<size_t> hits_{0};
    mutable std::atomic<size_t> misses_{0};
    std::atomic<size_t> max_capacity_;
    std::atomic<size_t> current_memory_usage_{0};

    // Helper methods
    bool evict_if_needed();
    void update_statistics(bool hit);
};

} // namespace cache
