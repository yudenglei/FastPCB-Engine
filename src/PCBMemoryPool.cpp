/**
 * @file PCBMemoryPool.cpp
 * @brief 内存池实现
 */

#include "PCBMemoryPool.h"
#include <algorithm>

namespace FastPCB {

// ============================================================================
// ParameterPool Implementation
// ============================================================================

// ============================================================================
// VariablePool Implementation  
// ============================================================================

VariablePool::VariablePool() : nextFree_(0) {
    pool_.resize(MemoryConfig::PARAM_POOL_SIZE);
}

uint32_t VariablePool::createOrGet(const std::string& name, double value) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    uint64_t hash = hashName(name);
    auto it = nameHashToIndex_.find(hash);
    
    if (it != nameHashToIndex_.end()) {
        return it->second;  // 已存在
    }
    
    if (nextFree_ >= pool_.size()) {
        return UINT32_MAX;  // 池已满
    }
    
    uint32_t idx = nextFree_++;
    
    // 添加到池
    pool_[idx].nameOffset_ = strPool_.add(name.c_str());
    pool_[idx].value_ = value;
    pool_[idx].defaultValue_ = value;
    pool_[idx].isExpression_ = false;
    pool_[idx].isValid_ = true;
    pool_[idx].refCount_ = 0;
    
    // 建立索引
    nameHashToIndex_[hash] = idx;
    
    return idx;
}

void VariablePool::setValue(uint32_t index, double value) {
    if (index >= pool_.size()) return;
    std::lock_guard<std::mutex> lock(mutex_);
    pool_[index].value_ = value;
}

double VariablePool::getValue(uint32_t index) const {
    if (index >= pool_.size()) return 0.0;
    return pool_[index].value_;
}

void VariablePool::setExpression(uint32_t index, const std::string& expression) {
    if (index >= pool_.size()) return;
    std::lock_guard<std::mutex> lock(mutex_);
    pool_[index].isExpression_ = true;
    // 表达式求值后续实现
}

double VariablePool::evaluate(uint32_t index) {
    if (index >= pool_.size()) return 0.0;
    
    std::lock_guard<std::mutex> lock(mutex_);
    auto& var = pool_[index];
    
    if (var.isExpression_) {
        // 表达式求值
        std::string name = strPool_.getString(var.nameOffset_);
        return evaluateExpression(name);
    }
    
    return var.value_;
}

void VariablePool::setValuesBatch(const std::vector<uint32_t>& indices, 
                                  const std::vector<double>& values) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    size_t count = std::min(indices.size(), values.size());
    for (size_t i = 0; i < count; ++i) {
        if (indices[i] < pool_.size()) {
            pool_[indices[i]].value_ = values[i];
        }
    }
}

std::string VariablePool::getName(uint32_t index) const {
    if (index >= pool_.size()) return "";
    return strPool_.getString(pool_[index].nameOffset_);
}

bool VariablePool::exists(const std::string& name) const {
    uint64_t hash = hashName(name);
    return nameHashToIndex_.find(hash) != nameHashToIndex_.end();
}

int32_t VariablePool::getIndex(const std::string& name) const {
    uint64_t hash = hashName(name);
    auto it = nameHashToIndex_.find(hash);
    if (it != nameHashToIndex_.end()) {
        return static_cast<int32_t>(it->second);
    }
    return -1;
}

uint64_t VariablePool::hashName(const std::string& name) const {
    uint64_t hash = 5381;
    for (char c : name) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

double VariablePool::evaluateExpression(const std::string& expr) {
    // 简化实现：支持基本算术表达式
    // 后续可以集成数学表达式库
    return 0.0;
}

// ============================================================================
// ParamRef Implementation
// ============================================================================

void ParamRef::setDouble(double value, VariablePool* pool) {
    // 创建一个临时变量
    char name[32];
    snprintf(name, sizeof(name), "_temp_%u", poolIndex_);
    poolIndex_ = pool->createOrGet(name, value);
    flags_ = 0;  // 清除变量标志
}

void ParamRef::setVariable(const std::string& name, VariablePool* pool) {
    poolIndex_ = pool->createOrGet(name, 0.0);
    flags_ = 0x01;  // 设置变量标志
}

void ParamRef::setExpression(const std::string& expr, VariablePool* pool) {
    char name[32];
    snprintf(name, sizeof(name), "_expr_%u", poolIndex_);
    poolIndex_ = pool->createOrGet(name, 0.0);
    pool->setExpression(poolIndex_, expr);
    flags_ = 0x03;  // 变量 + 表达式
}

double ParamRef::getValue(VariablePool* pool) const {
    if (!pool) return 0.0;
    return pool->getValue(poolIndex_);
}

} // namespace FastPCB
