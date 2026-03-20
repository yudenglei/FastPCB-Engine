/**
 * @file PCBVia.cpp
 * @brief PCB过孔实现
 */

#include "PCBVia.h"
#include "PCBVariablePool.h"

namespace FastPCB {

// ============================================================================
// PCBVia Implementation
// ============================================================================

PCBVia::PCBVia()
    : id_(0), label_(0),
      xIndex_(0), yIndex_(0),
      padDiameterIndex_(0), drillDiameterIndex_(0),
      topLayerID_(0), bottomLayerID_(0),
      startLayerID_(0), endLayerID_(0),
      netID_(0),
      padTopGeomID_(0), padBotGeomID_(0), drillGeomID_(0),
      flags_(0), padCount_(0) {
}

void PCBVia::create(uint32_t id, uint32_t label, VariablePool* pool) {
    id_ = id;
    label_ = label;
    
    // 预创建坐标参数
    if (pool) {
        char name[64];
        snprintf(name, sizeof(name), "via_%u_x", id);
        xIndex_ = pool->createOrGet(name, 0.0);
        
        snprintf(name, sizeof(name), "via_%u_y", id);
        yIndex_ = pool->createOrGet(name, 0.0);
        
        snprintf(name, sizeof(name), "via_%u_pad", id);
        padDiameterIndex_ = pool->createOrGet(name, 0.3);  // 默认0.3mm
        
        snprintf(name, sizeof(name), "via_%u_drill", id);
        drillDiameterIndex_ = pool->createOrGet(name, 0.2);  // 默认0.2mm
    }
}

void PCBVia::setPosition(double x, double y, VariablePool* pool) {
    if (!pool) return;
    pool->setValue(xIndex_, x);
    pool->setValue(yIndex_, y);
}

void PCBVia::getPosition(double& x, double& y, VariablePool* pool) const {
    if (!pool) return;
    x = pool->getValue(xIndex_);
    y = pool->getValue(yIndex_);
}

void PCBVia::setPadDiameter(double diameter, VariablePool* pool) {
    if (!pool) return;
    pool->setValue(padDiameterIndex_, diameter);
}

double PCBVia::getPadDiameter(VariablePool* pool) const {
    if (!pool) return 0.0;
    return pool->getValue(padDiameterIndex_);
}

void PCBVia::setDrillDiameter(double diameter, VariablePool* pool) {
    if (!pool) return;
    pool->setValue(drillDiameterIndex_, diameter);
}

double PCBVia::getDrillDiameter(VariablePool* pool) const {
    if (!pool) return 0.0;
    return pool->getValue(drillDiameterIndex_);
}

void PCBVia::setLayers(uint32_t startLayer, uint32_t endLayer) {
    startLayerID_ = startLayer;
    endLayerID_ = endLayer;
}

void PCBVia::getLayers(uint32_t& startLayer, uint32_t& endLayer) const {
    startLayer = startLayerID_;
    endLayer = endLayerID_;
}

// ============================================================================
// ViaPool Implementation
// ============================================================================

ViaPool::ViaPool() : nextID_(0) {
}

PCBVia* ViaPool::create(uint32_t label, VariablePool* pool) {
    uint32_t id = nextID_++;
    auto via = std::make_unique<PCBVia>();
    via->create(id, label, pool);
    vias_.push_back(std::move(via));
    return vias_.back().get();
}

PCBVia* ViaPool::get(uint32_t id) {
    if (id >= vias_.size()) return nullptr;
    return vias_[id].get();
}

void ViaPool::remove(uint32_t id) {
    if (id >= vias_.size()) return;
    vias_[id].reset();
    freeList_.push_back(id);
}

std::vector<PCBVia*> ViaPool::createBatch(uint32_t count, uint32_t startLabel, VariablePool* pool) {
    std::vector<PCBVia*> result;
    result.reserve(count);
    
    for (uint32_t i = 0; i < count; ++i) {
        result.push_back(create(startLabel + i, pool));
    }
    
    return result;
}

} // namespace FastPCB
