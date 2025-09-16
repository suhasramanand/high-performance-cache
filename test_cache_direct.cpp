#include "include/cache.h"
#include <iostream>
#include <chrono>

int main() {
    std::cout << "Testing cache functionality directly..." << std::endl;
    
    cache::Cache cache(1024 * 1024); // 1MB cache
    
    // Test basic operations
    std::cout << "1. Testing SET operation..." << std::endl;
    bool set_result = cache.set("key1", "value1");
    std::cout << "   SET result: " << (set_result ? "SUCCESS" : "FAILED") << std::endl;
    
    std::cout << "2. Testing GET operation..." << std::endl;
    std::string value = cache.get("key1");
    std::cout << "   GET result: '" << value << "'" << std::endl;
    
    std::cout << "3. Testing cache size..." << std::endl;
    std::cout << "   Cache size: " << cache.size() << std::endl;
    
    std::cout << "4. Testing statistics..." << std::endl;
    std::cout << "   Hits: " << cache.hits() << std::endl;
    std::cout << "   Misses: " << cache.misses() << std::endl;
    std::cout << "   Hit ratio: " << cache.hit_ratio() << std::endl;
    
    std::cout << "5. Testing multiple operations..." << std::endl;
    cache.set("key2", "value2");
    cache.set("key3", "value3");
    std::cout << "   Cache size after adding more keys: " << cache.size() << std::endl;
    
    std::cout << "6. Testing GET operations..." << std::endl;
    std::cout << "   key1: '" << cache.get("key1") << "'" << std::endl;
    std::cout << "   key2: '" << cache.get("key2") << "'" << std::endl;
    std::cout << "   key3: '" << cache.get("key3") << "'" << std::endl;
    
    std::cout << "7. Testing performance..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 10000; ++i) {
        std::string key = "perf_key_" + std::to_string(i % 100);
        std::string val = "perf_value_" + std::to_string(i);
        cache.set(key, val);
        cache.get(key);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "   10,000 operations completed in " << duration.count() << " microseconds" << std::endl;
    std::cout << "   Final cache size: " << cache.size() << std::endl;
    std::cout << "   Final hit ratio: " << cache.hit_ratio() << std::endl;
    
    std::cout << "\n✅ Cache functionality test completed successfully!" << std::endl;
    
    return 0;
}
