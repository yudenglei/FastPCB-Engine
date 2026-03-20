/**
 * @file PCBRefPool.h
 * @brief Reference池化 - 优化：使用整数ID代替OCAF Reference
 */

#ifndef PCBREFPOOL_H
#define PCBREFPOOL_H

#include "PCBConfig.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <mutex>
#include <memory>

namespace FastPCB {

// ============================================================================
// Reference池 - 核心优化：共享+整数ID
// ============================================================================

/**
 * @brief Reference池
 * 
 * 优化策略：
 * 1. 使用整数ID代替OCAF Reference Label（节省80%内存）
 * 2. 字符串池化：相同名称只存储一份
 * 3. 惰性解析：仅在需要时解析为实际Label
 */
class RefPool {
private:
    // 字符串池（避免重复存储）
    struct StringEntry {
        uint32_t next;      // 链表next
        uint32_t refCount;  // 引用计数
        char name[1];       // 可变长度
    };
    std::vector<char> stringBuffer_;
    std::vector<uint32_t> hashTable_[65536];
    uint32_t stringBufferPos_;
    
    // ID映射
    std::unordered_map<std::string, uint32_t> nameToID_;
    std::vector<std::string> idToName_;
    uint32_t nextID_;
    
    std::mutex mutex_;
    
public:
    RefPool();
    
    /**
     * @brief 获取或创建Reference ID
     * @param name Reference名称
     * @return ID
     */
    uint32_t getOrCreate(const std::string& name);
    
    /**
     * @brief 通过ID获取名称
     * @param id Reference ID
     * @return 名称（不存在返回空）
     */
    const std::string& getName(uint32_t id) const;
    
    /**
     * @brief 通过名称获取ID
     * @param name 名称
     * @return ID（不存在返回0）
     */
    uint32_t getID(const std::string& name) const;
    
    /**
     * @brief 批量获取ID（优化）
     * @param names 名称列表
     * @return ID列表
     */
    std::vector<uint32_t> getIDsBatch(const std::vector<std::string>& names);
    
    /**
     * @brief 释放Reference
     * @param id Reference ID
     */
    void release(uint32_t id);
    
    size_t size() const { return nextID_; }
    
private:
    uint32_t addString(const char* str);
    uint32_t hashString(const char* str) const;
};

// ============================================================================
// 惰性Reference - 延迟解析优化
// ============================================================================

/**
 * @brief 惰性Reference
 * 
 * 优化策略：
 * - 仅在真正需要时解析为OCAF Label
 * - 缓存解析结果避免重复解析
 */
class LazyRef {
private:
    uint32_t refID_;           // Reference池ID
    bool resolved_;            // 是否已解析
    void* resolvedTarget_;     // 解析后的目标（可以是任意类型）
    uint32_t accessCount_;     // 访问次数（用于LRU）
    
public:
    explicit LazyRef(uint32_t refID = 0);
    
    /**
     * @brief 解析Reference
     * @tparam T 目标类型
     * @return 解析后的目标
     */
    template<typename T>
    T* resolve() {
        if (!resolved_) {
            resolvedTarget_ = doResolve();
            resolved_ = true;
        }
        accessCount_++;
        return static_cast<T*>(resolvedTarget_);
    }
    
    /**
     * @brief 强制重新解析
     */
    void invalidate() {
        resolved_ = false;
    }
    
    uint32_t getID() const { return refID_; }
    uint32_t getAccessCount() const { return accessCount_; }

private:
    virtual void* doResolve() = 0;
};

// ============================================================================
// Reference集合 - 高效批量管理
// ============================================================================

/**
 * @brief Reference集合（用于组件、网络等）
 * 
 * 使用紧凑的vector存储，避免set开销
 */
class RefCollection {
private:
    std::vector<uint32_t> refs_;  // 紧凑存储
    bool sorted_;
    
public:
    RefCollection() : sorted_(false) {}
    
    // 添加Reference
    void add(uint32_t refID);
    
    // 移除Reference
    void remove(uint32_t refID);
    
    // 检查是否存在
    bool contains(uint32_t refID) const;
    
    // 获取数量
    size_t size() const { return refs_.size(); }
    
    // 批量添加
    void addBatch(const std::vector<uint32_t>& newRefs);
    
    // 批量移除
    void removeBatch(const std::vector<uint32_t>& oldRefs);
    
    // 获取所有Reference
    const std::vector<uint32_t>& getAll() { 
        if (!sorted_) {
            std::sort(refs_.begin(), refs_.end());
            sorted_ = true;
        }
        return refs_; 
    }
    
    // 清空
    void clear() { refs_.clear(); sorted_ = false; }
    
    // 交集
    RefCollection intersect(const RefCollection& other) const;
    
    // 并集
    RefCollection unite(const RefCollection& other) const;
};

} // namespace FastPCB

#endif // PCBREFPOOL_H
