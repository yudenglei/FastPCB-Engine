/**
 * @file PCBSurface.cpp
 * @brief PCB铜皮实现
 */

#include "PCBSurface.h"
#include <cmath>

namespace FastPCB {

// ============================================================================
// PCBSurface Implementation
// ============================================================================

PCBSurface::PCBSurface()
    : id_(0), label_(0),
      layerID_(0), netID_(0), geometryID_(0),
      type_(0), flags_(0), area_(0.0) {
}

void PCBSurface::create(uint32_t id, uint32_t label, VariablePool* pool) {
    id_ = id;
    label_ = label;
}

void PCBSurface::setBoundary(const std::vector<double>& points) {
    boundary_ = points;
}

void PCBSurface::addBoundaryPoint(double x, double y) {
    boundary_.push_back(x);
    boundary_.push_back(y);
}

void PCBSurface::addHole(const std::vector<double>& points) {
    holes_.push_back(points);
}

void PCBSurface::removeHole(size_t index) {
    if (index < holes_.size()) {
        holes_.erase(holes_.begin() + index);
    }
}

void PCBSurface::calculateArea() {
    if (boundary_.size() < 4) {
        area_ = 0.0;
        return;
    }
    
    double area = 0.0;
    size_t n = boundary_.size() / 2;
    
    for (size_t i = 0; i < n; ++i) {
        size_t j = (i + 1) % n;
        area += boundary_[2*i] * boundary_[2*j + 1];
        area -= boundary_[2*i + 1] * boundary_[2*j];
    }
    
    area_ = std::abs(area) / 2.0;
    
    // 减去挖孔面积
    for (const auto& hole : holes_) {
        if (hole.size() >= 4) {
            double holeArea = 0.0;
            size_t m = hole.size() / 2;
            for (size_t i = 0; i < m; ++i) {
                size_t j = (i + 1) % m;
                holeArea += hole[2*i] * hole[2*j + 1];
                holeArea -= hole[2*i + 1] * hole[2*j];
            }
            holeArea = std::abs(holeArea) / 2.0;
            area_ -= holeArea;
        }
    }
}

// ============================================================================
// SurfacePool Implementation
// ============================================================================

SurfacePool::SurfacePool() : nextID_(0) {
}

PCBSurface* SurfacePool::create(uint32_t label, VariablePool* pool) {
    uint32_t id = nextID_++;
    auto surface = std::make_unique<PCBSurface>();
    surface->create(id, label, pool);
    
    surfaces_.push_back(std::move(surface));
    return surfaces_.back().get();
}

PCBSurface* SurfacePool::get(uint32_t id) {
    if (id >= surfaces_.size()) return nullptr;
    return surfaces_[id].get();
}

void SurfacePool::remove(uint32_t id) {
    if (id >= surfaces_.size()) return;
    surfaces_[id].reset();
    freeList_.push_back(id);
}

std::vector<PCBSurface*> SurfacePool::getByNet(uint32_t netID) {
    std::vector<PCBSurface*> result;
    for (auto& surf : surfaces_) {
        if (surf && surf->getNet() == netID) {
            result.push_back(surf.get());
        }
    }
    return result;
}

std::vector<PCBSurface*> SurfacePool::getByLayer(uint32_t layerID) {
    std::vector<PCBSurface*> result;
    for (auto& surf : surfaces_) {
        if (surf && surf->getLayer() == layerID) {
            result.push_back(surf.get());
        }
    }
    return result;
}

} // namespace FastPCB
