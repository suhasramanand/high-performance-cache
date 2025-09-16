#pragma once

#include <memory>
#include <queue>
#include <mutex>
#include <atomic>
#include <functional>

namespace cache {

template<typename T>
class ObjectPool {
public:
    explicit ObjectPool(size_t initial_size = 100, size_t max_size = 1000)
        : max_size_(max_size), created_count_(0) {
        for (size_t i = 0; i < initial_size; ++i) {
            pool_.push(std::make_unique<T>());
        }
    }

    ~ObjectPool() = default;

    // Non-copyable, non-movable
    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;
    ObjectPool(ObjectPool&&) = delete;
    ObjectPool& operator=(ObjectPool&&) = delete;

    template<typename... Args>
    std::unique_ptr<T> acquire(Args&&... args) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!pool_.empty()) {
            auto obj = std::move(pool_.front());
            pool_.pop();
            return obj;
        }
        
        if (created_count_ < max_size_) {
            ++created_count_;
            return std::make_unique<T>(std::forward<Args>(args)...);
        }
        
        // Pool is full, create temporary object
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    void release(std::unique_ptr<T> obj) {
        if (!obj) return;
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (pool_.size() < max_size_) {
            // Reset object state if it has a reset method
            // Note: This is a simplified version without C++20 concepts
            pool_.push(std::move(obj));
        }
        // If pool is full, object will be destroyed
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return pool_.size();
    }

    size_t created_count() const {
        return created_count_.load();
    }

private:
    mutable std::mutex mutex_;
    std::queue<std::unique_ptr<T>> pool_;
    size_t max_size_;
    std::atomic<size_t> created_count_;
};

} // namespace cache
