#pragma once

#include <unordered_map>
#include <list>
#include <mutex>
#include <optional>

namespace cache {

template<typename Key, typename Value>
class LRUCache {
public:
    explicit LRUCache(size_t capacity) : capacity_(capacity) {}

    // Non-copyable, non-movable
    LRUCache(const LRUCache&) = delete;
    LRUCache& operator=(const LRUCache&) = delete;
    LRUCache(LRUCache&&) = delete;
    LRUCache& operator=(LRUCache&&) = delete;

    std::optional<Value> get(const Key& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = cache_map_.find(key);
        if (it == cache_map_.end()) {
            return std::nullopt;
        }
        
        // Move to front (most recently used)
        access_order_.splice(access_order_.begin(), access_order_, it->second);
        
        // Return a copy of the value
        return std::make_optional(it->second->second);
    }

    void put(const Key& key, Value value) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = cache_map_.find(key);
        if (it != cache_map_.end()) {
            // Update existing entry
            it->second->second = std::move(value);
            access_order_.splice(access_order_.begin(), access_order_, it->second);
            return;
        }
        
        // Add new entry
        if (cache_map_.size() >= capacity_) {
            // Remove least recently used
            auto lru_it = std::prev(access_order_.end());
            cache_map_.erase(lru_it->first);
            access_order_.erase(lru_it);
        }
        
        access_order_.emplace_front(key, std::move(value));
        cache_map_[key] = access_order_.begin();
    }

    bool remove(const Key& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = cache_map_.find(key);
        if (it == cache_map_.end()) {
            return false;
        }
        
        access_order_.erase(it->second);
        cache_map_.erase(it);
        return true;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_map_.clear();
        access_order_.clear();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return cache_map_.size();
    }

    size_t capacity() const {
        return capacity_;
    }

    void set_capacity(size_t capacity) {
        std::lock_guard<std::mutex> lock(mutex_);
        capacity_ = capacity;
        
        // Evict excess entries
        while (cache_map_.size() > capacity_) {
            auto lru_it = std::prev(access_order_.end());
            cache_map_.erase(lru_it->first);
            access_order_.erase(lru_it);
        }
    }

private:
    mutable std::mutex mutex_;
    std::list<std::pair<Key, Value>> access_order_;
    std::unordered_map<Key, typename std::list<std::pair<Key, Value>>::iterator> cache_map_;
    size_t capacity_;
};

} // namespace cache
