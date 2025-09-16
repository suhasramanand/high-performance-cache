#include <gtest/gtest.h>
#include "memory_allocator.h"
#include <vector>
#include <thread>

class MemoryAllocatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        allocator_ = std::make_unique<cache::MemoryAllocator>(1024 * 1024); // 1MB pool
    }
    
    void TearDown() override {
        allocator_.reset();
    }
    
    std::unique_ptr<cache::MemoryAllocator> allocator_;
};

TEST_F(MemoryAllocatorTest, BasicAllocation) {
    void* ptr = allocator_->allocate(100);
    EXPECT_NE(ptr, nullptr);
    
    allocator_->deallocate(ptr, 100);
}

TEST_F(MemoryAllocatorTest, MultipleAllocations) {
    std::vector<void*> ptrs;
    
    // Allocate multiple blocks
    for (int i = 0; i < 10; ++i) {
        void* ptr = allocator_->allocate(50);
        EXPECT_NE(ptr, nullptr);
        ptrs.push_back(ptr);
    }
    
    // Deallocate all blocks
    for (void* ptr : ptrs) {
        allocator_->deallocate(ptr, 50);
    }
}

TEST_F(MemoryAllocatorTest, AllocationStatistics) {
    EXPECT_EQ(allocator_->allocation_count(), 0);
    EXPECT_EQ(allocator_->allocated_bytes(), 0);
    
    void* ptr1 = allocator_->allocate(100);
    EXPECT_EQ(allocator_->allocation_count(), 1);
    EXPECT_EQ(allocator_->allocated_bytes(), 100);
    
    void* ptr2 = allocator_->allocate(200);
    EXPECT_EQ(allocator_->allocation_count(), 2);
    EXPECT_EQ(allocator_->allocated_bytes(), 300);
    
    allocator_->deallocate(ptr1, 100);
    EXPECT_EQ(allocator_->allocation_count(), 2); // Count doesn't decrease
    EXPECT_EQ(allocator_->allocated_bytes(), 200);
    
    allocator_->deallocate(ptr2, 200);
    EXPECT_EQ(allocator_->allocated_bytes(), 0);
}

TEST_F(MemoryAllocatorTest, LargeAllocation) {
    size_t large_size = 1024 * 1024; // 1MB
    void* ptr = allocator_->allocate(large_size);
    EXPECT_NE(ptr, nullptr);
    
    allocator_->deallocate(ptr, large_size);
}

TEST_F(MemoryAllocatorTest, FragmentationRatio) {
    // Allocate and deallocate to create fragmentation
    std::vector<void*> ptrs;
    
    // Allocate some blocks
    for (int i = 0; i < 5; ++i) {
        void* ptr = allocator_->allocate(100);
        ptrs.push_back(ptr);
    }
    
    // Deallocate every other block
    for (int i = 0; i < 5; i += 2) {
        allocator_->deallocate(ptrs[i], 100);
    }
    
    double fragmentation = allocator_->fragmentation_ratio();
    EXPECT_GE(fragmentation, 0.0);
    EXPECT_LE(fragmentation, 1.0);
}

TEST_F(MemoryAllocatorTest, ConcurrentAllocation) {
    const int num_threads = 4;
    const int allocations_per_thread = 100;
    
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([this, allocations_per_thread, &success_count]() {
            std::vector<void*> ptrs;
            
            for (int i = 0; i < allocations_per_thread; ++i) {
                void* ptr = allocator_->allocate(50);
                if (ptr != nullptr) {
                    ptrs.push_back(ptr);
                    success_count++;
                }
            }
            
            // Deallocate all allocated blocks
            for (void* ptr : ptrs) {
                allocator_->deallocate(ptr, 50);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(success_count.load(), num_threads * allocations_per_thread);
}

TEST_F(MemoryAllocatorTest, TotalBytes) {
    size_t initial_total = allocator_->total_bytes();
    EXPECT_GT(initial_total, 0);
    
    // Allocate some memory
    void* ptr = allocator_->allocate(1000);
    EXPECT_NE(ptr, nullptr);
    
    // Total bytes should remain the same (pool size)
    EXPECT_EQ(allocator_->total_bytes(), initial_total);
    
    allocator_->deallocate(ptr, 1000);
}

TEST_F(MemoryAllocatorTest, Alignment) {
    // Test that allocations are properly aligned
    for (int i = 0; i < 10; ++i) {
        size_t size = 1 + i * 7; // Various sizes
        void* ptr = allocator_->allocate(size);
        EXPECT_NE(ptr, nullptr);
        
        // Check alignment (should be aligned to 8-byte boundary)
        uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
        EXPECT_EQ(addr % 8, 0);
        
        allocator_->deallocate(ptr, size);
    }
}

TEST_F(MemoryAllocatorTest, NullDeallocation) {
    // Deallocating nullptr should not crash
    allocator_->deallocate(nullptr, 100);
    
    // Should still be able to allocate after null deallocation
    void* ptr = allocator_->allocate(100);
    EXPECT_NE(ptr, nullptr);
    allocator_->deallocate(ptr, 100);
}
