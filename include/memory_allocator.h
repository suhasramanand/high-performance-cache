#pragma once

#include <memory>
#include <vector>
#include <mutex>
#include <atomic>

namespace cache {

class MemoryAllocator {
public:
    explicit MemoryAllocator(size_t pool_size = 1024 * 1024); // 1MB default
    ~MemoryAllocator();

    // Non-copyable, non-movable
    MemoryAllocator(const MemoryAllocator&) = delete;
    MemoryAllocator& operator=(const MemoryAllocator&) = delete;
    MemoryAllocator(MemoryAllocator&&) = delete;
    MemoryAllocator& operator=(MemoryAllocator&&) = delete;

    // Memory allocation
    void* allocate(size_t size);
    void deallocate(void* ptr, size_t size);
    
    // Statistics
    size_t allocated_bytes() const;
    size_t total_bytes() const;
    size_t allocation_count() const;
    double fragmentation_ratio() const;

private:
    struct Block {
        void* ptr;
        size_t size;
        bool in_use;
        
        Block(void* p, size_t s) : ptr(p), size(s), in_use(false) {}
    };

    mutable std::mutex mutex_;
    std::vector<std::unique_ptr<char[]>> pools_;
    std::vector<Block> blocks_;
    
    std::atomic<size_t> allocated_bytes_{0};
    std::atomic<size_t> allocation_count_{0};
    size_t pool_size_;
    size_t current_pool_offset_{0};

    void allocate_new_pool();
    Block* find_free_block(size_t size);
    void merge_adjacent_blocks();
};

} // namespace cache
