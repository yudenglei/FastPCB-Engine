/**
 * @file PCBTrace.cpp
 * @brief PCB走线实现
 */

#include "PCBTrace.h"
#include "PCBVariablePool.h"

namespace FastPCB {

// ============================================================================
// PCBTrace Implementation
// ============================================================================

PCBTrace::PCBTrace()
    : id_(0), label_(0), widthIndex_(0),
      layerID_(0), netID_(0), geometryID_(0), flags_(0) {
}

void PCBTrace::create(uint32_t id, uint32_t label, VariablePool* pool) {
    id_ = id;
    label_ = label;
    
    if (pool) {
        char name[64];
        snprintf(name, sizeof(name), "trace_%u_w", id);
        widthIndex_ = pool->createOrGet(name, 0.2);  // 默认0.2mm线宽
    }
}

void PCBTrace::setPath(const std::vector<double>& points) {
    pathPoints_ = points;
}

void PCBTrace::addPoint(double x, double y) {
    pathPoints_.push_back(x);
    pathPoints_.push_back(y);
}

void PCBTrace::setWidth(double width, VariablePool* pool) {
    if (!pool) return;
    if (widthIndex_ == 0) {
        char name[64];
        snprintf(name, sizeof(name), "trace_%u_w", id_);
        widthIndex_ = pool->createOrGet(name, width);
    } else {
        pool->setValue(widthIndex_, width);
    }
}

double PCBTrace::getWidth(VariablePool* pool) const {
    if (!pool || widthIndex_ == 0) return 0.0;
    return pool->getValue(widthIndex_);
}

// ============================================================================
// TracePool Implementation
// ============================================================================

TracePool::TracePool() : nextID_(0) {
}

PCBTrace* TracePool::create(uint32_t label, VariablePool* pool) {
    uint32_t id = nextID_++;
    auto trace = std::make_unique<PCBTrace>();
    trace->create(id, label, pool);
    
    // 索引
    if (trace->getNet() > 0) {
        netIndex_[trace->getNet()].push_back(id);
    }
    if (trace->getLayer() > 0) {
        layerIndex_[trace->getLayer()].push_back(id);
    }
    
    traces_.push_back(std::move(trace));
    return traces_.back().get();
}

PCBTrace* TracePool::get(uint32_t id) {
    if (id >= traces_.size()) return nullptr;
    return traces_[id].get();
}

void TracePool::remove(uint32_t id) {
    if (id >= traces_.size()) return;
    
    auto& trace = traces_[id];
    if (trace) {
        // 从索引中移除
        if (trace->getNet() > 0) {
            auto& netVec = netIndex_[trace->getNet()];
            netVec.erase(std::remove(netVec.begin(), netVec.end(), id), netVec.end());
        }
        if (trace->getLayer() > 0) {
            auto& layerVec = layerIndex_[trace->getLayer()];
            layerVec.erase(std::remove(layerVec.begin(), layerVec.end(), id), layerVec.end());
        }
    }
    
    traces_[id].reset();
    freeList_.push_back(id);
}

std::vector<PCBTrace*> TracePool::getByNet(uint32_t netID) {
    std::vector<PCBTrace*> result;
    auto it = netIndex_.find(netID);
    if (it != netIndex_.end()) {
        for (uint32_t id : it->second) {
            if (id < traces_.size() && traces_[id]) {
                result.push_back(traces_[id].get());
            }
        }
    }
    return result;
}

std::vector<PCBTrace*> TracePool::getByLayer(uint32_t layerID) {
    std::vector<PCBTrace*> result;
    auto it = layerIndex_.find(layerID);
    if (it != layerIndex_.end()) {
        for (uint32_t id : it->second) {
            if (id < traces_.size() && traces_[id]) {
                result.push_back(traces_[id].get());
            }
        }
    }
    return result;
}

} // namespace FastPCB
