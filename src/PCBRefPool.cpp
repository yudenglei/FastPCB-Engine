/**
 * @file PCBRefPool.cpp
 * @brief Reference池实现
 */

#include "PCBRefPool.h"
#include <algorithm>

namespace FastPCB {

// ============================================================================
// RefPool Implementation
// ============================================================================

RefPool::RefPool() : nextID_(1), stringBufferPos_(0) {
    stringBuffer_.resize(MemoryConfig::REF_POOL_SIZE * 64);  // 预分配
}

uint32_t RefPool::getOrCreate(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = nameToID_.find(name);
    if (it != nameToID_.end()) {
        return it->second;
    }
    
    uint32_t id = nextID_++;
    nameToID_[name] = id;
    
    if (id >= idToName_.size()) {
        idToName_.resize(id + 1);
    }
    idToName_[id] = name;
    
    return id;
}

const std::string& RefPool::getName(uint32_t id) const {
    static std::string empty;
    if (id == 0 || id >= idToName_.size()) {
        return empty;
    }
    return idToName_[id];
}

uint32_t RefPool::getID(const std::string& name) const {
    auto it = nameToID_.find(name);
    if (it != nameToID_.end()) {
        return it->second;
    }
    return 0;
}

std::vector<uint32_t> RefPool::getIDsBatch(const std::vector<std::string>& names) {
    std::vector<uint32_t> result;
    result.reserve(names.size());
    
    for (const auto& name : names) {
        result.push_back(getOrCreate(name));
    }
    
    return result;
}

void RefPool::release(uint32_t id) {
    // Reference池化，暂不真正释放
    // 后续可以添加引用计数
}

uint32_t RefPool::addString(const char* str) {
    size_t len = strlen(str) + 1;
    if (stringBufferPos_ + len > stringBuffer_.size()) {
        return 0;
    }
    
    uint32_t offset = stringBufferPos_;
    memcpy(&stringBuffer_[stringBufferPos_], str, len);
    stringBufferPos_ += len;
    
    return offset;
}

uint32_t RefPool::hashString(const char* str) const {
    uint32_t hash = 5381;
    while (*str) {
        hash = ((hash << 5) + hash) + *str++;
    }
    return hash;
}

// ============================================================================
// LazyRef Implementation
// ============================================================================

LazyRef::LazyRef(uint32_t refID) 
    : refID_(refID), resolved_(false), resolvedTarget_(nullptr), accessCount_(0) {
}

// ============================================================================
// RefCollection Implementation
// ============================================================================

void RefCollection::add(uint32_t refID) {
    if (!contains(refID)) {
        refs_.push_back(refID);
        sorted_ = false;
    }
}

void RefCollection::remove(uint32_t refID) {
    auto it = std::find(refs_.begin(), refs_.end(), refID);
    if (it != refs_.end()) {
        refs_.erase(it);
    }
}

bool RefCollection::contains(uint32_t refID) const {
    return std::find(refs_.begin(), refs_.end(), refID) != refs_.end();
}

void RefCollection::addBatch(const std::vector<uint32_t>& newRefs) {
    for (uint32_t ref : newRefs) {
        add(ref);
    }
}

void RefCollection::removeBatch(const std::vector<uint32_t>& oldRefs) {
    for (uint32_t ref : oldRefs) {
        remove(ref);
    }
}

RefCollection RefCollection::intersect(const RefCollection& other) const {
    RefCollection result;
    
    for (uint32_t ref : refs_) {
        if (other.contains(ref)) {
            result.add(ref);
        }
    }
    
    return result;
}

RefCollection RefCollection::unite(const RefCollection& other) const {
    RefCollection result = *this;
    
    for (uint32_t ref : other.refs_) {
        result.add(ref);
    }
    
    return result;
}

} // namespace FastPCB
