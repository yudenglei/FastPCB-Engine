/**
 * @file PCBVariablePool.h
 * @brief 参数化变量池 - 核心优化：延迟创建+池化
 */

#ifndef PCBVARIABLEPOOL_H
#define PCBVARIABLEPOOL_H

#include "PCBMemoryPool.h"
#include <string>
#include <unordered_map>
#include <mutex>
#include <functional>

namespace FastPCB {

// ============================================================================
// 参数化变量
// ============================================================================

/**
 * @brief 参数化变量（存储在变量池中）
 * 
 * 优化策略：
 * - 使用紧凑存储（结构体而非类）
 * - 预分配大量槽位，避免频繁new
 * - 支持表达式求值
 */
struct Variable {
    uint32_t nameOffset_;      // 字符串池偏移
    double value_;             // 当前值
    double defaultValue_;      // 默认值
    bool isExpression_;       // 是否为表达式
    bool isValid_;             // 是否有效
    uint32_t refCount_;        // 引用计数（用于缓存）
    
    Variable() : nameOffset_(0), value_(0), defaultValue_(0), 
                 isExpression_(false), isValid_(false), refCount_(0) {}
};

// ============================================================================
// 变量池 - 核心数据结构
// ============================================================================

/**
 * @brief 参数化变量池
 * 
 * 核心优化：
 * 1. 预分配100万个槽位
 * 2. 使用整数索引代替Label
 * 3. 延迟创建：仅当真正使用时才创建
 * 4. 表达式缓存：避免重复解析
 */
class VariablePool {
private:
    // 预分配的变量数组（核心优化）
    std::vector<Variable> pool_;
    
    // 稀疏索引：变量名Hash -> 池索引
    std::unordered_map<uint64_t, uint32_t> nameHashToIndex_;
    
    // 字符串池（避免重复存储变量名）
    StringPool strPool_;
    
    // 表达式缓存
    struct ExpressionCache {
        std::string expr;
        double cachedValue;
        uint64_t exprHash;
    };
    std::vector<ExpressionCache> exprCache_;
    
    std::mutex mutex_;
    uint32_t nextFree_;
    
public:
    VariablePool();
    
    /**
     * @brief 创建或获取变量
     * @param name 变量名
     * @param value 初始值
     * @return 变量池索引
     */
    uint32_t createOrGet(const std::string& name, double value = 0.0);
    
    /**
     * @brief 设置变量值
     * @param index 变量索引
     * @param value 新值
     */
    void setValue(uint32_t index, double value);
    
    /**
     * @brief 获取变量值
     * @param index 变量索引
     * @return 变量值
     */
    double getValue(uint32_t index) const;
    
    /**
     * @brief 设置表达式
     * @param index 变量索引
     * @param expression 表达式字符串
     */
    void setExpression(uint32_t index, const std::string& expression);
    
    /**
     * @brief 求值（支持表达式）
     * @param index 变量索引
     * @return 计算结果
     */
    double evaluate(uint32_t index);
    
    /**
     * @brief 批量设置变量（优化）
     * @param indices 索引数组
     * @param values 值数组
     */
    void setValuesBatch(const std::vector<uint32_t>& indices, 
                        const std::vector<double>& values);
    
    /**
     * @brief 获取变量名
     * @param index 变量索引
     * @return 变量名
     */
    std::string getName(uint32_t index) const;
    
    /**
     * @brief 检查变量是否存在
     * @param name 变量名
     * @return 是否存在
     */
    bool exists(const std::string& name) const;
    
    /**
     * @brief 获取变量索引（不存在返回-1）
     * @param name 变量名
     * @return 索引或-1
     */
    int32_t getIndex(const std::string& name) const;
    
    size_t size() const { return nextFree_; }
    size_t capacity() const { return pool_.size(); }

private:
    uint64_t hashName(const std::string& name) const;
    double evaluateExpression(const std::string& expr);
};

// ============================================================================
// 参数引用 - 指向变量池中的变量
// ============================================================================

/**
 * @brief 参数引用（用于器件参数）
 * 
 * 优化策略：
 * - 使用整数索引（4字节）代替Label引用
 * - 延迟创建变量
 */
class ParamRef {
private:
    uint32_t poolIndex_;       // 变量池索引（4字节）
    uint16_t flags_;           // 标志（2字节）
    uint16_t reserved_;        // 对齐（2字节）
    
public:
    ParamRef() : poolIndex_(0), flags_(0), reserved_(0) {}
    
    // 设置为double值
    void setDouble(double value, VariablePool* pool);
    
    // 设置为变量引用
    void setVariable(const std::string& name, VariablePool* pool);
    
    // 设置为表达式
    void setExpression(const std::string& expr, VariablePool* pool);
    
    // 获取值
    double getValue(VariablePool* pool) const;
    
    // 检查类型
    bool isVariable() const { return flags_ & 0x01; }
    bool isExpression() const { return flags_ & 0x02; }
    bool isDouble() const { return !isVariable() && !isExpression(); }
    
    // 标记脏
    void markDirty() { flags_ |= 0x04; }
    void clearDirty() { flags_ &= ~0x04; }
    bool isDirty() const { return flags_ & 0x04; }
    
    uint32_t getPoolIndex() const { return poolIndex_; }
};

} // namespace FastPCB

#endif // PCBVARIABLEPOOL_H
