#include "memory_allocator.h"
#include <cstring>
#include <algorithm>
#include <iostream>

namespace cache {

MemoryAllocator::MemoryAllocator(size_t pool_size) 
    : pool_size_(pool_size) {
    allocate_new_pool();
}

MemoryAllocator::~MemoryAllocator() {
    // Memory will be automatically freed when pools_ goes out of scope
}

void* MemoryAllocator::allocate(size_t size) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Store original size for accurate tracking
    size_t original_size = size;
    
    // Align size to 8-byte boundary for better performance
    size = (size + 7) & ~7;
    
    // Try to find a free block
    auto block = find_free_block(size);
    if (block) {
        block->in_use = true;
        allocated_bytes_ += original_size; // Track original size, not aligned size
        allocation_count_++;
        return block->ptr;
    }
    
    // If no free block found, allocate new pool if needed
    if (current_pool_offset_ + size > pool_size_) {
        allocate_new_pool();
    }
    
    // Allocate from current pool
    void* ptr = pools_.back().get() + current_pool_offset_;
    current_pool_offset_ += size;
    
    // Add to blocks list
    blocks_.emplace_back(ptr, size);
    blocks_.back().in_use = true;
    
    allocated_bytes_ += original_size; // Track original size, not aligned size
    allocation_count_++;
    
    return ptr;
}

void MemoryAllocator::deallocate(void* ptr, size_t size) {
    if (!ptr) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Find the block
    auto it = std::find_if(blocks_.begin(), blocks_.end(),
        [ptr](const Block& block) { return block.ptr == ptr; });
    
    if (it != blocks_.end()) {
        it->in_use = false;
        allocated_bytes_ -= size;
        
        // Try to merge with adjacent free blocks
        merge_adjacent_blocks();
    }
}

size_t MemoryAllocator::allocated_bytes() const {
    return allocated_bytes_.load();
}

size_t MemoryAllocator::total_bytes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return pools_.size() * pool_size_;
}

size_t MemoryAllocator::allocation_count() const {
    return allocation_count_.load();
}

double MemoryAllocator::fragmentation_ratio() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    size_t free_bytes = 0;
    for (const auto& block : blocks_) {
        if (!block.in_use) {
            free_bytes += block.size;
        }
    }
    
    size_t total_bytes = pools_.size() * pool_size_;
    if (total_bytes == 0) return 0.0;
    
    return static_cast<double>(free_bytes) / total_bytes;
}

void MemoryAllocator::allocate_new_pool() {
    auto pool = std::make_unique<char[]>(pool_size_);
    pools_.push_back(std::move(pool));
    current_pool_offset_ = 0;
}

MemoryAllocator::Block* MemoryAllocator::find_free_block(size_t size) {
    for (auto& block : blocks_) {
        if (!block.in_use && block.size >= size) {
            return &block;
        }
    }
    return nullptr;
}

void MemoryAllocator::merge_adjacent_blocks() {
    // Sort blocks by address
    std::sort(blocks_.begin(), blocks_.end(),
        [](const Block& a, const Block& b) { return a.ptr < b.ptr; });
    
    // Merge adjacent free blocks
    for (auto it = blocks_.begin(); it != blocks_.end(); ) {
        if (!it->in_use) {
            auto next = it + 1;
            while (next != blocks_.end() && !next->in_use &&
                   static_cast<char*>(it->ptr) + it->size == next->ptr) {
                it->size += next->size;
                next = blocks_.erase(next);
            }
        }
        ++it;
    }
}

} // namespace cache
