#include <gtest/gtest.h>
#include "lru_cache.h"
#include <string>

class LRUCacheTest : public ::testing::Test {
protected:
    void SetUp() override {
        cache_ = std::make_unique<cache::LRUCache<std::string, std::string>>(3);
    }
    
    void TearDown() override {
        cache_.reset();
    }
    
    std::unique_ptr<cache::LRUCache<std::string, std::string>> cache_;
};

TEST_F(LRUCacheTest, BasicPutGet) {
    cache_->put("key1", "value1");
    auto result = cache_->get("key1");
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "value1");
}

TEST_F(LRUCacheTest, GetNonExistentKey) {
    auto result = cache_->get("nonexistent");
    EXPECT_FALSE(result.has_value());
}

TEST_F(LRUCacheTest, OverwriteKey) {
    cache_->put("key1", "value1");
    cache_->put("key1", "value2");
    
    auto result = cache_->get("key1");
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "value2");
}

TEST_F(LRUCacheTest, CapacityLimit) {
    // Add entries up to capacity
    cache_->put("key1", "value1");
    cache_->put("key2", "value2");
    cache_->put("key3", "value3");
    
    EXPECT_EQ(cache_->size(), 3);
    
    // Add one more entry - should evict least recently used
    cache_->put("key4", "value4");
    
    EXPECT_EQ(cache_->size(), 3);
    
    // key1 should be evicted (least recently used)
    EXPECT_FALSE(cache_->get("key1").has_value());
    
    // key2, key3, key4 should still be there
    EXPECT_TRUE(cache_->get("key2").has_value());
    EXPECT_TRUE(cache_->get("key3").has_value());
    EXPECT_TRUE(cache_->get("key4").has_value());
}

TEST_F(LRUCacheTest, LRUEvictionOrder) {
    // Add entries
    cache_->put("key1", "value1");
    cache_->put("key2", "value2");
    cache_->put("key3", "value3");
    
    // Access key1 to make it most recently used
    cache_->get("key1");
    
    // Add new entry - key2 should be evicted (least recently used)
    cache_->put("key4", "value4");
    
    EXPECT_FALSE(cache_->get("key2").has_value());
    EXPECT_TRUE(cache_->get("key1").has_value());
    EXPECT_TRUE(cache_->get("key3").has_value());
    EXPECT_TRUE(cache_->get("key4").has_value());
}

TEST_F(LRUCacheTest, RemoveKey) {
    cache_->put("key1", "value1");
    cache_->put("key2", "value2");
    
    EXPECT_TRUE(cache_->remove("key1"));
    EXPECT_FALSE(cache_->get("key1").has_value());
    EXPECT_TRUE(cache_->get("key2").has_value());
    EXPECT_EQ(cache_->size(), 1);
}

TEST_F(LRUCacheTest, RemoveNonExistentKey) {
    EXPECT_FALSE(cache_->remove("nonexistent"));
}

TEST_F(LRUCacheTest, ClearCache) {
    cache_->put("key1", "value1");
    cache_->put("key2", "value2");
    
    EXPECT_EQ(cache_->size(), 2);
    
    cache_->clear();
    EXPECT_EQ(cache_->size(), 0);
    EXPECT_FALSE(cache_->get("key1").has_value());
    EXPECT_FALSE(cache_->get("key2").has_value());
}

TEST_F(LRUCacheTest, SetCapacity) {
    cache_->put("key1", "value1");
    cache_->put("key2", "value2");
    cache_->put("key3", "value3");
    
    EXPECT_EQ(cache_->size(), 3);
    
    // Reduce capacity
    cache_->set_capacity(2);
    EXPECT_EQ(cache_->size(), 2);
    EXPECT_EQ(cache_->capacity(), 2);
    
    // key1 should be evicted (least recently used)
    EXPECT_FALSE(cache_->get("key1").has_value());
    EXPECT_TRUE(cache_->get("key2").has_value());
    EXPECT_TRUE(cache_->get("key3").has_value());
}

TEST_F(LRUCacheTest, AccessOrderMaintenance) {
    // Add entries
    cache_->put("key1", "value1");
    cache_->put("key2", "value2");
    cache_->put("key3", "value3");
    
    // Access key2 to make it most recently used
    cache_->get("key2");
    
    // Add new entry - key1 should be evicted
    cache_->put("key4", "value4");
    
    EXPECT_FALSE(cache_->get("key1").has_value());
    EXPECT_TRUE(cache_->get("key2").has_value());
    EXPECT_TRUE(cache_->get("key3").has_value());
    EXPECT_TRUE(cache_->get("key4").has_value());
}

TEST_F(LRUCacheTest, UpdateExistingKey) {
    cache_->put("key1", "value1");
    cache_->put("key2", "value2");
    cache_->put("key3", "value3");
    
    // Update key1 - should make it most recently used
    cache_->put("key1", "value1_updated");
    
    // Add new entry - key2 should be evicted
    cache_->put("key4", "value4");
    
    EXPECT_TRUE(cache_->get("key1").has_value());
    EXPECT_EQ(cache_->get("key1").value(), "value1_updated");
    EXPECT_FALSE(cache_->get("key2").has_value());
    EXPECT_TRUE(cache_->get("key3").has_value());
    EXPECT_TRUE(cache_->get("key4").has_value());
}

TEST_F(LRUCacheTest, IntegerKeys) {
    auto int_cache = cache::LRUCache<int, std::string>(2);
    
    int_cache.put(1, "one");
    int_cache.put(2, "two");
    
    EXPECT_TRUE(int_cache.get(1).has_value());
    EXPECT_EQ(int_cache.get(1).value(), "one");
    
    int_cache.put(3, "three"); // Should evict key 2
    
    EXPECT_FALSE(int_cache.get(2).has_value());
    EXPECT_TRUE(int_cache.get(1).has_value());
    EXPECT_TRUE(int_cache.get(3).has_value());
}
