#include "cache.h"
#include <algorithm>
#include <iostream>

namespace cache {

Cache::Cache(size_t max_capacity) 
    : lru_cache_(10000), // Start with 10k entries capacity
      allocator_(std::make_unique<MemoryAllocator>()),
      entry_pool_(std::make_unique<ObjectPool<CacheEntry>>()),
      max_capacity_(max_capacity) {
}

bool Cache::set(const std::string& key, const std::string& value) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    // Check if we need to evict entries
    size_t entry_size = key.size() + value.size() + sizeof(CacheEntry);
    
    // If the single entry is larger than capacity, reject it
    if (entry_size > max_capacity_) {
        return false;
    }
    
    if (current_memory_usage_ + entry_size > max_capacity_) {
        if (!evict_if_needed()) {
            return false; // Couldn't free enough space
        }
    }
    
    // Create new entry
    CacheEntry entry(key, value);
    
    // Store in LRU cache
    lru_cache_.put(key, std::move(entry));
    current_memory_usage_ += entry_size;
    
    return true;
}

std::string Cache::get(const std::string& key) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    auto entry_opt = lru_cache_.get(key);
    if (!entry_opt.has_value()) {
        update_statistics(false);
        return "";
    }
    
    auto entry = std::move(entry_opt.value());
    std::string value = entry.value; // Copy the value first
    entry.access_count++;
    entry.timestamp = std::chrono::steady_clock::now();
    
    // Put the updated entry back
    lru_cache_.put(key, std::move(entry));
    
    update_statistics(true);
    return value;
}

bool Cache::remove(const std::string& key) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    auto entry_opt = lru_cache_.get(key);
    if (!entry_opt.has_value()) {
        return false;
    }
    
    auto entry = std::move(entry_opt.value());
    size_t entry_size = key.size() + entry.value.size() + sizeof(CacheEntry);
    
    if (lru_cache_.remove(key)) {
        current_memory_usage_ -= entry_size;
        return true;
    }
    
    return false;
}

void Cache::clear() {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    lru_cache_.clear();
    current_memory_usage_ = 0;
}

size_t Cache::size() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return lru_cache_.size();
}

size_t Cache::capacity() const {
    return max_capacity_.load();
}

double Cache::hit_ratio() const {
    size_t total = hits_.load() + misses_.load();
    if (total == 0) return 0.0;
    return static_cast<double>(hits_.load()) / total;
}

size_t Cache::hits() const {
    return hits_.load();
}

size_t Cache::misses() const {
    return misses_.load();
}

size_t Cache::memory_usage() const {
    return current_memory_usage_.load();
}

void Cache::set_max_capacity(size_t capacity) {
    max_capacity_ = capacity;
    
    // If current usage exceeds new capacity, evict entries
    if (current_memory_usage_ > capacity) {
        evict_if_needed();
    }
}

bool Cache::evict_if_needed() {
    // Simple eviction: remove oldest entries until we have enough space
    size_t target_usage = max_capacity_ * 0.8; // Keep at 80% capacity
    
    while (current_memory_usage_ > target_usage && lru_cache_.size() > 0) {
        // Reduce LRU cache capacity to force eviction
        size_t current_size = lru_cache_.size();
        if (current_size > 1) {
            lru_cache_.set_capacity(current_size - 1);
            // Estimate memory reduction (this is simplified)
            size_t estimated_reduction = 50; // Rough estimate for small entries
            if (current_memory_usage_ > estimated_reduction) {
                current_memory_usage_ -= estimated_reduction;
            } else {
                current_memory_usage_ = 0;
            }
        } else {
            break;
        }
    }
    
    return current_memory_usage_ <= target_usage;
}

void Cache::update_statistics(bool hit) {
    if (hit) {
        hits_++;
    } else {
        misses_++;
    }
}

} // namespace cache
