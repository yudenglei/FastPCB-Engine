/**
 * @file PCBMemoryPool.h
 * @brief 内存池 - 核心优化：预分配+对象池，减少malloc/free
 */

#ifndef PCBMEMORYPOOL_H
#define PCBMEMORYPOOL_H

#include "PCBConfig.h"
#include <vector>
#include <memory>
#include <atomic>
#include <cstring>

namespace FastPCB {

// ============================================================================
// 紧凑参数值 - 核心优化：用union实现8字节存储
// ============================================================================

/**
 * @brief 紧凑参数值（16字节对齐）
 * 
 * 优化策略：
 * - 未设置变量：直接存储double值（8字节）
 * - 设置变量：存储变量池索引（4字节）+ 标志（2字节）
 * - 相比传统Label方案节省80%+内存
 */
struct ParamValue {
private:
    union {
        double doubleValue_;      // 未设置变量时：8字节
        uint32_t varPoolIndex_;   // 设置变量时：4字节
    };
    uint16_t flags_;              // 标志：2字节
    uint16_t reserved_;           // 填充：2字节（对齐到16字节）
    
public:
    ParamValue() : doubleValue_(0.0), flags_(0), reserved_(0) {}
    
    // 检查是否为变量
    bool isVariable() const { return flags_ & ParamFlags::IS_VARIABLE; }
    
    // 检查是否为表达式
    bool isExpression() const { return flags_ & ParamFlags::IS_EXPRESSION; }
    
    // 检查是否被引用
    bool isReferenced() const { return flags_ & ParamFlags::IS_REFERENCED; }
    
    // 检查是否脏（需要刷新）
    bool isDirty() const { return flags_ & ParamFlags::IS_DIRTY; }
    
    // 获取double值
    double getDouble() const { return doubleValue_; }
    
    // 获取变量池索引
    uint32_t getVarIndex() const { return varPoolIndex_; }
    
    // 设置double值
    void setDouble(double v) { 
        doubleValue_ = v; 
        flags_ &= ~ParamFlags::IS_VARIABLE;  // 清除变量标志
        flags_ &= ~ParamFlags::IS_EXPRESSION;
    }
    
    // 设置变量索引
    void setVariable(uint32_t idx, bool isExpr = false) {
        varPoolIndex_ = idx;
        flags_ |= ParamFlags::IS_VARIABLE;
        if (isExpr) flags_ |= ParamFlags::IS_EXPRESSION;
    }
    
    // 设置脏标志
    void setDirty(bool dirty) {
        if (dirty) flags_ |= ParamFlags::IS_DIRTY;
        else flags_ &= ~ParamFlags::IS_DIRTY;
    }
    
    // 设置引用标志
    void setReferenced(bool ref) {
        if (ref) flags_ |= ParamFlags::IS_REFERENCED;
        else flags_ &= ~ParamFlags::IS_REFERENCED;
    }
};

// ============================================================================
// 内存块 - 预分配的大块内存
// ============================================================================

template<typename T>
class MemoryBlock {
private:
    T* data_;
    size_t capacity_;
    std::atomic<size_t> used_;
    
public:
    explicit MemoryBlock(size_t capacity) 
        : capacity_(capacity), used_(0) {
        data_ = static_cast<T*>(::operator new(capacity_ * sizeof(T)));
    }
    
    ~MemoryBlock() {
        ::operator delete(data_);
    }
    
    // 分配（线程安全）
    T* allocate(size_t count = 1) {
        size_t oldUsed = used_.fetch_add(count);
        if (oldUsed + count > capacity_) {
            return nullptr;  // 内存不足
        }
        return &data_[oldUsed];
    }
    
    // 重置
    void reset() { used_.store(0); }
    
    size_t capacity() const { return capacity_; }
    size_t used() const { return used_.load(); }
    size_t available() const { return capacity_ - used_; }
    
    T* data() const { return data_; }
    
    // 禁止拷贝
    MemoryBlock(const MemoryBlock&) = delete;
    MemoryBlock& operator=(const MemoryBlock&) = delete;
};

// ============================================================================
// 对象池 - 减少new/delete开销
// ============================================================================

template<typename T>
class ObjectPool {
private:
    std::vector<T*> freeList_;
    std::vector<MemoryBlock<T>*> blocks_;
    size_t blockSize_;
    std::mutex mutex_;
    
public:
    explicit ObjectPool(size_t blockSize = 1024) : blockSize_(blockSize) {
        allocateBlock();
    }
    
    ~ObjectPool() {
        for (auto* block : blocks_) {
            delete block;
        }
    }
    
    T* allocate() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!freeList_.empty()) {
            T* obj = freeList_.back();
            freeList_.pop_back();
            return obj;
        }
        
        // 尝试在新块中分配
        T* obj = blocks_.back()->allocate();
        if (!obj) {
            allocateBlock();
            obj = blocks_.back()->allocate();
        }
        
        return obj;
    }
    
    void deallocate(T* obj) {
        std::lock_guard<std::mutex> lock(mutex_);
        freeList_.push_back(obj);
    }
    
    size_t totalAllocated() const {
        size_t total = 0;
        for (auto* block : blocks_) {
            total += block->used();
        }
        return total;
    }
    
private:
    void allocateBlock() {
        blocks_.push_back(new MemoryBlock<T>(blockSize_));
    }
};

// ============================================================================
// 参数池 - 核心数据结构
// ============================================================================

class ParameterPool {
private:
    // 预分配的参数数组（核心优化：连续内存）
    std::vector<ParamValue> params_;
    std::atomic<uint32_t> nextFree_;
    
    // 稀疏索引：LabelID -> PoolIndex
    std::vector<int32_t> labelToIndex_;
    
public:
    ParameterPool(size_t maxParams = MemoryConfig::PARAM_POOL_SIZE) 
        : params_(maxParams), nextFree_(0) {
        labelToIndex_.resize(2000000, -1);  // 支持200万个Label
    }
    
    // 分配参数（返回索引）
    uint32_t allocate(int labelId) {
        uint32_t idx = nextFree_++;
        if (idx >= params_.size()) {
            return UINT32_MAX;  // 池已满
        }
        
        // 建立Label到索引的映射
        if (labelId < static_cast<int>(labelToIndex_.size())) {
            labelToIndex_[labelId] = static_cast<int32_t>(idx);
        }
        
        return idx;
    }
    
    // 通过Label获取参数
    ParamValue* getParam(int labelId) {
        if (labelId < 0 || labelId >= static_cast<int>(labelToIndex_.size())) {
            return nullptr;
        }
        int32_t idx = labelToIndex_[labelId];
        if (idx < 0 || idx >= static_cast<int>(params_.size())) {
            return nullptr;
        }
        return &params_[idx];
    }
    
    // 通过索引获取参数
    ParamValue* getParamByIndex(uint32_t idx) {
        if (idx >= params_.size()) return nullptr;
        return &params_[idx];
    }
    
    // 批量设置double值（批量优化）
    void setDoubleBatch(const std::vector<int>& labelIds, double value) {
        for (int labelId : labelIds) {
            auto* param = getParam(labelId);
            if (param) param->setDouble(value);
        }
    }
    
    size_t used() const { return nextFree_.load(); }
    size_t capacity() const { return params_.size(); }
};

// ============================================================================
// 字符串池 - 减少字符串重复存储
// ============================================================================

class StringPool {
private:
    struct StringEntry {
        uint32_t hash_;
        uint32_t next_;  // 链表next
        char data_[1];   // 可变长度
    };
    
    std::vector<char> buffer_;
    std::vector<uint32_t> hashTable_[65536];  // 16位哈希表
    uint32_t bufferPos_;
    
public:
    StringPool(size_t size = MemoryConfig::STRING_POOL_SIZE) 
        : buffer_(size), bufferPos_(0) {}
    
    // 添加字符串（返回偏移量）
    uint32_t add(const char* str) {
        uint32_t hash = hashString(str);
        uint32_t bucket = hash & 0xFFFF;
        
        // 查找是否已存在
        for (uint32_t idx : hashTable_[bucket]) {
            if (strcmp(getString(idx), str) == 0) {
                return idx;  // 已存在，返回已有
            }
        }
        
        // 新建
        size_t len = strlen(str) + 1;
        if (bufferPos_ + len > buffer_.size()) {
            return 0;  // 池已满
        }
        
        uint32_t offset = bufferPos_;
        memcpy(&buffer_[bufferPos_], str, len);
        bufferPos_ += len;
        
        // 加入哈希表
        StringEntry* entry = reinterpret_cast<StringEntry*>(&buffer_[offset]);
        entry->hash_ = hash;
        entry->next_ = 0;
        
        // 添加到链表
        if (hashTable_[bucket].empty()) {
            hashTable_[bucket].push_back(offset);
        } else {
            // 简单实现：添加到末尾
            hashTable_[bucket].push_back(offset);
        }
        
        return offset;
    }
    
    const char* getString(uint32_t offset) const {
        if (offset >= bufferPos_) return "";
        return &buffer_[offset];
    }
    
private:
    uint32_t hashString(const char* str) {
        uint32_t hash = 5381;
        while (*str) {
            hash = ((hash << 5) + hash) + *str++;
        }
        return hash;
    }
};

} // namespace FastPCB

#endif // PCBMEMORYPOOL_H
