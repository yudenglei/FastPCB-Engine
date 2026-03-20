/**
 * @file PCBLoader.h
 * @brief 批量数据加载器 - 高性能优化
 */

#ifndef PCBLOADER_H
#define PCBLOADER_H

#include "PCBDataModel.h"
#include <vector>
#include <string>
#include <memory>
#include <thread>
#include <future>
#include <atomic>

namespace FastPCB {

// ============================================================================
// 批量加载器 - 高性能优化
// ============================================================================

/**
 * @brief 批量数据加载器
 * 
 * 优化策略：
 * 1. 多线程并行加载
 * 2. 批量提交减少事务开销
 * 3. 预分配内存减少分配
 * 4. 延迟索引构建
 */
class PCBLoader {
private:
    PCBDataModel* model_;
    
    // 加载统计
    std::atomic<uint64_t> loadedComponents_;
    std::atomic<uint64_t> loadedVias_;
    std::atomic<uint64_t> loadedTraces_;
    std::atomic<uint64_t> loadedSurfaces_;
    
    // 线程数
    int threadCount_;
    
public:
    explicit PCBLoader(PCBDataModel* model, int threads = 8);
    ~PCBLoader() = default;
    
    // ==================== 批量加载 ====================
    
    /**
     * @brief 批量加载Via
     */
    void loadViasBatch(const std::vector<double>& x,
                       const std::vector<double>& y,
                       const std::vector<double>& padDiameter,
                       const std::vector<double>& drillDiameter,
                       const std::vector<uint32_t>& netIds);
    
    /**
     * @brief 批量加载Trace
     */
    void loadTracesBatch(const std::vector<std::vector<double>>& paths,
                        const std::vector<double>& widths,
                        const std::vector<uint32_t>& layerIds,
                        const std::vector<uint32_t>& netIds);
    
    /**
     * @brief 批量加载Surface
     */
    void loadSurfacesBatch(const std::vector<std::vector<double>>& boundaries,
                          const std::vector<std::vector<std::vector<double>>>& holes,
                          const std::vector<uint32_t>& layerIds,
                          const std::vector<uint32_t>& netIds);
    
    /**
     * @brief 批量加载Component
     */
    void loadComponentsBatch(ComponentType type,
                           const std::vector<double>& x,
                           const std::vector<double>& y,
                           const std::vector<double>& rotation,
                           const std::vector<uint32_t>& netIds);
    
    // ==================== 文件加载 ====================
    
    /**
     * @brief 从文件批量导入
     */
    bool importFromFile(const std::string& filename);
    
    /**
     * @brief 从GenCAD导入
     */
    bool importGenCAD(const std::string& filename);
    
    /**
     * @brief 从DXF导入
     */
    bool importDXF(const std::string& filename);
    
    // ==================== 统计 ====================
    
    uint64_t getLoadedCount() const {
        return loadedComponents_ + loadedVias_ + loadedTraces_ + loadedSurfaces_;
    }
    
    void resetStats() {
        loadedComponents_ = 0;
        loadedVias_ = 0;
        loadedTraces_ = 0;
        loadedSurfaces_ = 0;
    }

private:
    void loadViasParallel(const std::vector<double>& x,
                         const std::vector<double>& y,
                         size_t start, size_t end);
    
    void loadTracesParallel(const std::vector<std::vector<double>>& paths,
                           const std::vector<double>& widths,
                           size_t start, size_t end);
};

// ============================================================================
// 高效增量更新器
// ============================================================================

/**
 * @brief 增量更新器
 * 
 * 优化策略：
 * 1. 最小化OCAF事务
 * 2. 批量参数更新
 * 3. 选择性Shape刷新
 */
class IncrementalUpdater {
private:
    PCBDataModel* model_;
    std::vector<uint32_t> dirtyParams_;
    std::vector<double> paramValues_;
    
public:
    explicit IncrementalUpdater(PCBDataModel* model) : model_(model) {}
    
    /**
     * @brief 标记参数为脏
     */
    void markDirty(uint32_t paramIndex, double value) {
        dirtyParams_.push_back(paramIndex);
        paramValues_.push_back(value);
    }
    
    /**
     * @brief 批量提交更新
     */
    void commit();
    
    /**
     * @brief 刷新所有脏参数关联的Shape
     */
    void refreshShapes();
    
    /**
     * @brief 清空脏标记
     */
    void clear() {
        dirtyParams_.clear();
        paramValues_.clear();
    }
};

// ============================================================================
// LRU缓存 - Shape缓存优化
// ============================================================================

/**
 * @brief LRU缓存模板
 */
template<typename Key, typename Value>
class LRUCache {
private:
    struct CacheEntry {
        Value value;
        uint64_t accessTime;
        bool isValid;
    };
    
    std::unordered_map<Key, CacheEntry> cache_;
    size_t capacity_;
    uint64_t accessCounter_;
    std::mutex mutex_;
    
public:
    explicit LRUCache(size_t capacity) : capacity_(capacity), accessCounter_(0) {}
    
    /**
     * @brief 获取缓存
     */
    bool get(const Key& key, Value& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = cache_.find(key);
        if (it != cache_.end() && it->second.isValid) {
            it->second.accessTime = ++accessCounter_;
            value = it->second.value;
            return true;
        }
        return false;
    }
    
    /**
     * @brief 设置缓存
     */
    void put(const Key& key, const Value& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (cache_.size() >= capacity_) {
            // 淘汰最旧的
            uint64_t oldest = UINT64_MAX;
            Key oldestKey;
            
            for (auto& pair : cache_) {
                if (pair.second.accessTime < oldest) {
                    oldest = pair.second.accessTime;
                    oldestKey = pair.first;
                }
            }
            
            cache_.erase(oldestKey);
        }
        
        cache_[key] = {value, ++accessCounter_, true};
    }
    
    /**
     * @brief 清除缓存
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_.clear();
    }
    
    size_t size() const { return cache_.size(); }
};

// ============================================================================
// 内存压缩器
// ============================================================================

/**
 * @brief 内存压缩器
 * 
 * 优化策略：
 * 1. 冷数据压缩
 * 2. 重复数据去重
 * 3. 稀疏存储
 */
class MemoryCompactor {
private:
    PCBDataModel* model_;
    
public:
    explicit MemoryCompactor(PCBDataModel* model) : model_(model) {}
    
    /**
     * @brief 压缩冷数据
     */
    void compactColdData();
    
    /**
     * @brief 去重重复数据
     */
    void deduplicate();
    
    /**
     * @brief 回收空闲内存
     */
    void shrink();
    
    /**
     * @brief 获取压缩率
     */
    double getCompressionRatio() const;
};

} // namespace FastPCB

#endif // PCBLOADER_H
