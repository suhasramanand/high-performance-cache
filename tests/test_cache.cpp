#include <gtest/gtest.h>
#include "cache.h"
#include <thread>
#include <vector>
#include <random>

class CacheTest : public ::testing::Test {
protected:
    void SetUp() override {
        cache_ = std::make_unique<cache::Cache>(1024 * 1024); // 1MB capacity
    }
    
    void TearDown() override {
        cache_.reset();
    }
    
    std::unique_ptr<cache::Cache> cache_;
};

TEST_F(CacheTest, BasicSetGet) {
    EXPECT_TRUE(cache_->set("key1", "value1"));
    EXPECT_EQ(cache_->get("key1"), "value1");
    EXPECT_EQ(cache_->size(), 1);
}

TEST_F(CacheTest, GetNonExistentKey) {
    EXPECT_EQ(cache_->get("nonexistent"), "");
    EXPECT_EQ(cache_->misses(), 1);
}

TEST_F(CacheTest, OverwriteKey) {
    EXPECT_TRUE(cache_->set("key1", "value1"));
    EXPECT_TRUE(cache_->set("key1", "value2"));
    EXPECT_EQ(cache_->get("key1"), "value2");
    EXPECT_EQ(cache_->size(), 1);
}

TEST_F(CacheTest, RemoveKey) {
    EXPECT_TRUE(cache_->set("key1", "value1"));
    EXPECT_TRUE(cache_->remove("key1"));
    EXPECT_EQ(cache_->get("key1"), "");
    EXPECT_EQ(cache_->size(), 0);
}

TEST_F(CacheTest, RemoveNonExistentKey) {
    EXPECT_FALSE(cache_->remove("nonexistent"));
}

TEST_F(CacheTest, ClearCache) {
    EXPECT_TRUE(cache_->set("key1", "value1"));
    EXPECT_TRUE(cache_->set("key2", "value2"));
    EXPECT_EQ(cache_->size(), 2);
    
    cache_->clear();
    EXPECT_EQ(cache_->size(), 0);
    EXPECT_EQ(cache_->get("key1"), "");
    EXPECT_EQ(cache_->get("key2"), "");
}

TEST_F(CacheTest, HitRatioCalculation) {
    EXPECT_TRUE(cache_->set("key1", "value1"));
    
    // Hit
    EXPECT_EQ(cache_->get("key1"), "value1");
    EXPECT_EQ(cache_->hits(), 1);
    EXPECT_EQ(cache_->misses(), 0);
    EXPECT_DOUBLE_EQ(cache_->hit_ratio(), 1.0);
    
    // Miss
    EXPECT_EQ(cache_->get("key2"), "");
    EXPECT_EQ(cache_->hits(), 1);
    EXPECT_EQ(cache_->misses(), 1);
    EXPECT_DOUBLE_EQ(cache_->hit_ratio(), 0.5);
}

TEST_F(CacheTest, MemoryUsage) {
    EXPECT_TRUE(cache_->set("key1", "value1"));
    EXPECT_TRUE(cache_->set("key2", "value2"));
    
    size_t memory_usage = cache_->memory_usage();
    EXPECT_GT(memory_usage, 0);
    
    cache_->clear();
    EXPECT_EQ(cache_->memory_usage(), 0);
}

TEST_F(CacheTest, CapacityLimit) {
    // Set a very small capacity
    cache_->set_max_capacity(100);
    
    // Try to add a large value
    std::string large_value(200, 'x');
    EXPECT_FALSE(cache_->set("key1", large_value));
}

TEST_F(CacheTest, ConcurrentAccess) {
    const int num_threads = 4;
    const int operations_per_thread = 1000;
    
    std::vector<std::thread> threads;
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([this, t, operations_per_thread]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, 99);
            
            for (int i = 0; i < operations_per_thread; ++i) {
                int key_num = dis(gen);
                std::string key = "key_" + std::to_string(key_num);
                std::string value = "value_" + std::to_string(key_num) + "_" + std::to_string(t);
                
                if (i % 2 == 0) {
                    cache_->set(key, value);
                } else {
                    cache_->get(key);
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify cache is still functional
    EXPECT_TRUE(cache_->set("test_key", "test_value"));
    EXPECT_EQ(cache_->get("test_key"), "test_value");
}

TEST_F(CacheTest, LRUEviction) {
    // Set a small capacity to force eviction
    cache_->set_max_capacity(1000);
    
    // Add multiple entries
    for (int i = 0; i < 10; ++i) {
        std::string key = "key_" + std::to_string(i);
        std::string value = "value_" + std::to_string(i);
        EXPECT_TRUE(cache_->set(key, value));
    }
    
    // Access some keys to make them more recently used
    EXPECT_EQ(cache_->get("key_5"), "value_5");
    EXPECT_EQ(cache_->get("key_7"), "value_7");
    
    // Add more entries to trigger eviction
    for (int i = 10; i < 20; ++i) {
        std::string key = "key_" + std::to_string(i);
        std::string value = "value_" + std::to_string(i);
        cache_->set(key, value);
    }
    
    // Recently accessed keys should still be there
    EXPECT_EQ(cache_->get("key_5"), "value_5");
    EXPECT_EQ(cache_->get("key_7"), "value_7");
}

TEST_F(CacheTest, StatisticsAccuracy) {
    EXPECT_EQ(cache_->hits(), 0);
    EXPECT_EQ(cache_->misses(), 0);
    EXPECT_DOUBLE_EQ(cache_->hit_ratio(), 0.0);
    
    // Perform some operations
    cache_->set("key1", "value1");
    cache_->get("key1");  // hit
    cache_->get("key2");  // miss
    cache_->get("key1");  // hit
    
    EXPECT_EQ(cache_->hits(), 2);
    EXPECT_EQ(cache_->misses(), 1);
    EXPECT_DOUBLE_EQ(cache_->hit_ratio(), 2.0 / 3.0);
}
