/**
 * @file PCBComponent.cpp
 * @brief PCB器件实现
 */

#include "PCBComponent.h"
#include "PCBVariablePool.h"
#include <algorithm>
#include <stdexcept>

namespace FastPCB {

// ============================================================================
// PCBComponent Implementation
// ============================================================================

void PCBComponent::setPosition(double x, double y, VariablePool* pool) {
    if (!pool) return;
    
    // 使用变量池存储坐标
    if (xIndex_ == 0) {
        // 首次设置，创建变量
        char name[64];
        snprintf(name, sizeof(name), "comp_%u_x", id_);
        xIndex_ = pool->createOrGet(name, x);
        
        snprintf(name, sizeof(name), "comp_%u_y", id_);
        yIndex_ = pool->createOrGet(name, y);
    } else {
        pool->setValue(xIndex_, x);
        pool->setValue(yIndex_, y);
    }
}

void PCBComponent::getPosition(double& x, double& y, VariablePool* pool) const {
    if (!pool) return;
    
    x = (xIndex_ > 0) ? pool->getValue(xIndex_) : 0.0;
    y = (yIndex_ > 0) ? pool->getValue(yIndex_) : 0.0;
}

void PCBComponent::setRotation(double rotation, VariablePool* pool) {
    if (!pool) return;
    
    if (rotationIndex_ == 0) {
        char name[64];
        snprintf(name, sizeof(name), "comp_%u_rot", id_);
        rotationIndex_ = pool->createOrGet(name, rotation);
    } else {
        pool->setValue(rotationIndex_, rotation);
    }
}

double PCBComponent::getRotation(VariablePool* pool) const {
    if (!pool || rotationIndex_ == 0) return 0.0;
    return pool->getValue(rotationIndex_);
}

// ============================================================================
// ComponentPool Implementation
// ============================================================================

ComponentPool::ComponentPool(VariablePool* pool, size_t maxComponents)
    : varPool_(pool), nextId_(0) {
    components_.resize(maxComponents);
    labelToIndex_.resize(2000000, -1);
    typeIndices_.resize(9);  // ComponentType数量
}

uint32_t ComponentPool::create(ComponentType type, uint32_t label) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!freeList_.empty()) {
        uint32_t id = freeList_.back();
        freeList_.pop_back();
        
        auto& comp = components_[id];
        comp.id_ = id;
        comp.label_ = label;
        comp.type_ = type;
        
        labelToIndex_[label] = id;
        typeIndices_[static_cast<uint8_t>(type)].push_back(id);
        
        return id;
    }
    
    if (nextId_ >= components_.size()) {
        return UINT32_MAX;  // 池已满
    }
    
    uint32_t id = nextId_++;
    auto& comp = components_[id];
    comp.id_ = id;
    comp.label_ = label;
    comp.type_ = type;
    
    labelToIndex_[label] = id;
    typeIndices_[static_cast<uint8_t>(type)].push_back(id);
    
    return id;
}

PCBComponent* ComponentPool::get(uint32_t id) {
    if (id >= components_.size()) return nullptr;
    return &components_[id];
}

PCBComponent* ComponentPool::getByLabel(uint32_t label) {
    if (label >= labelToIndex_.size()) return nullptr;
    int32_t idx = labelToIndex_[label];
    if (idx < 0) return nullptr;
    return &components_[idx];
}

void ComponentPool::remove(uint32_t id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (id >= components_.size()) return;
    
    auto& comp = components_[id];
    labelToIndex_[comp.label_] = -1;
    
    // 从类型索引中移除
    auto& typeVec = typeIndices_[static_cast<uint8_t>(comp.type_)];
    typeVec.erase(std::remove(typeVec.begin(), typeVec.end(), id), typeVec.end());
    
    comp = PCBComponent();  // 重置
    freeList_.push_back(id);
}

std::vector<PCBComponent*> ComponentPool::getByNet(uint32_t netId) {
    std::vector<PCBComponent*> result;
    // 后续实现网络索引
    return result;
}

std::vector<PCBComponent*> ComponentPool::getByType(ComponentType type) {
    std::vector<PCBComponent*> result;
    auto& ids = typeIndices_[static_cast<uint8_t>(type)];
    result.reserve(ids.size());
    
    for (uint32_t id : ids) {
        result.push_back(&components_[id]);
    }
    
    return result;
}

std::vector<uint32_t> ComponentPool::createBatch(ComponentType type, uint32_t count, uint32_t startLabel) {
    std::vector<uint32_t> ids;
    ids.reserve(count);
    
    for (uint32_t i = 0; i < count; ++i) {
        uint32_t id = create(type, startLabel + i);
        if (id != UINT32_MAX) {
            ids.push_back(id);
        }
    }
    
    return ids;
}

void ComponentPool::removeBatch(const std::vector<uint32_t>& ids) {
    for (uint32_t id : ids) {
        remove(id);
    }
}

void ComponentPool::updateIndices(uint32_t id) {
    // 重建索引
}

void ComponentPool::removeFromIndex(uint32_t id) {
    // 从索引中移除
}

// ============================================================================
// ComponentFactory Implementation
// ============================================================================

std::unique_ptr<PCBComponent> ComponentFactory::create(ComponentType type) {
    return std::make_unique<PCBComponent>();
}

ComponentType ComponentFactory::stringToType(const std::string& str) {
    if (str == "IC") return ComponentType::IC;
    if (str == "RESISTOR") return ComponentType::RESISTOR;
    if (str == "CAPACITOR") return ComponentType::CAPACITOR;
    if (str == "CONNECTOR") return ComponentType::CONNECTOR;
    if (str == "VIA") return ComponentType::VIA;
    return ComponentType::IC;
}

std::string ComponentFactory::typeToString(ComponentType type) {
    switch (type) {
        case ComponentType::IC: return "IC";
        case ComponentType::RESISTOR: return "RESISTOR";
        case ComponentType::CAPACITOR: return "CAPACITOR";
        case ComponentType::INDUCTOR: return "INDUCTOR";
        case ComponentType::CONNECTOR: return "CONNECTOR";
        case ComponentType::VIA: return "VIA";
        default: return "UNKNOWN";
    }
}

} // namespace FastPCB
