/**
 * @file PCBBondWire.cpp
 * @brief BondWire实现
 */

#include "PCBBondWire.h"
#include "PCBVariablePool.h"
#include <cstring>

namespace FastPCB {

// ============================================================================
// PCBBondWire Implementation
// ============================================================================

PCBBondWire::PCBBondWire()
    : id_(0), label_(0), type_(BondWireType::WIRE_BOND),
      startXIndex_(0), startYIndex_(0), startZIndex_(0),
      endXIndex_(0), endYIndex_(0), endZIndex_(0),
      curveType_(CurveType::STRAIGHT),
      heightIndex_(0), angleIndex_(0), lengthIndex_(0),
      diameterIndex_(0), materialIndex_(0),
      netID_(0), sourceChipID_(0), targetPadID_(0),
      surfaceGeomID_(0), centerlineID_(0), flags_(0) {
}

void PCBBondWire::create(uint32_t id, uint32_t label, BondWireType type, VariablePool* pool) {
    id_ = id;
    label_ = label;
    type_ = type;
    
    if (pool) {
        char name[64];
        
        // 起点
        snprintf(name, sizeof(name), "bw_%u_sx", id);
        startXIndex_ = pool->createOrGet(name, 0.0);
        snprintf(name, sizeof(name), "bw_%u_sy", id);
        startYIndex_ = pool->createOrGet(name, 0.0);
        snprintf(name, sizeof(name), "bw_%u_sz", id);
        startZIndex_ = pool->createOrGet(name, 0.0);
        
        // 终点
        snprintf(name, sizeof(name), "bw_%u_ex", id);
        endXIndex_ = pool->createOrGet(name, 0.0);
        snprintf(name, sizeof(name), "bw_%u_ey", id);
        endYIndex_ = pool->createOrGet(name, 0.0);
        snprintf(name, sizeof(name), "bw_%u_ez", id);
        endZIndex_ = pool->createOrGet(name, 0.0);
        
        // 曲线参数
        snprintf(name, sizeof(name), "bw_%u_h", id);
        heightIndex_ = pool->createOrGet(name, 0.0);
        
        snprintf(name, sizeof(name), "bw_%u_d", id);
        diameterIndex_ = pool->createOrGet(name, 25.0);  // 默认25um线径
    }
}

void PCBBondWire::setStartPosition(double x, double y, double z, VariablePool* pool) {
    if (!pool) return;
    pool->setValue(startXIndex_, x);
    pool->setValue(startYIndex_, y);
    pool->setValue(startZIndex_, z);
}

void PCBBondWire::setEndPosition(double x, double y, double z, VariablePool* pool) {
    if (!pool) return;
    pool->setValue(endXIndex_, x);
    pool->setValue(endYIndex_, y);
    pool->setValue(endZIndex_, z);
}

void PCBBondWire::getStartPosition(double& x, double& y, double& z, VariablePool* pool) const {
    if (!pool) return;
    x = pool->getValue(startXIndex_);
    y = pool->getValue(startYIndex_);
    z = pool->getValue(startZIndex_);
}

void PCBBondWire::getEndPosition(double& x, double& y, double& z, VariablePool* pool) const {
    if (!pool) return;
    x = pool->getValue(endXIndex_);
    y = pool->getValue(endYIndex_);
    z = pool->getValue(endZIndex_);
}

void PCBBondWire::setHeight(double height, VariablePool* pool) {
    if (!pool) return;
    pool->setValue(heightIndex_, height);
}

double PCBBondWire::getHeight(VariablePool* pool) const {
    if (!pool) return 0.0;
    return pool->getValue(heightIndex_);
}

void PCBBondWire::setDiameter(double diameter, VariablePool* pool) {
    if (!pool) return;
    pool->setValue(diameterIndex_, diameter);
}

double PCBBondWire::getDiameter(VariablePool* pool) const {
    if (!pool) return 0.0;
    return pool->getValue(diameterIndex_);
}

void PCBBondWire::setMaterial(const std::string& material) {
    // 材料后续实现
}

// ============================================================================
// BondWirePool Implementation
// ============================================================================

BondWirePool::BondWirePool() : nextID_(0) {
}

PCBBondWire* BondWirePool::create(uint32_t label, BondWireType type, VariablePool* pool) {
    uint32_t id = nextID_++;
    auto wire = std::make_unique<PCBBondWire>();
    wire->create(id, label, type, pool);
    wires_.push_back(std::move(wire));
    return wires_.back().get();
}

PCBBondWire* BondWirePool::get(uint32_t id) {
    if (id >= wires_.size()) return nullptr;
    return wires_[id].get();
}

void BondWirePool::remove(uint32_t id) {
    if (id >= wires_.size()) return;
    wires_[id].reset();
    freeList_.push_back(id);
}

std::vector<PCBBondWire*> BondWirePool::getByNet(uint32_t netID) {
    std::vector<PCBBondWire*> result;
    for (auto& wire : wires_) {
        if (wire && wire->getNet() == netID) {
            result.push_back(wire.get());
        }
    }
    return result;
}

std::vector<PCBBondWire*> BondWirePool::getByChip(uint32_t chipID) {
    std::vector<PCBBondWire*> result;
    for (auto& wire : wires_) {
        if (wire && wire->getSource() == chipID) {
            result.push_back(wire.get());
        }
    }
    return result;
}

} // namespace FastPCB
