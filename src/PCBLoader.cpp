/**
 * @file PCBLoader.cpp
 * @brief 批量数据加载器实现
 */

#include "PCBLoader.h"
#include <iostream>
#include <algorithm>
#include <cstring>

namespace FastPCB {

// ============================================================================
// PCBLoader Implementation
// ============================================================================

PCBLoader::PCBLoader(PCBDataModel* model, int threads)
    : model_(model), threadCount_(threads),
      loadedComponents_(0), loadedVias_(0), 
      loadedTraces_(0), loadedSurfaces_(0) {
}

void PCBLoader::loadViasBatch(const std::vector<double>& x,
                              const std::vector<double>& y,
                              const std::vector<double>& padDiameter,
                              const std::vector<double>& drillDiameter,
                              const std::vector<uint32_t>& netIds) {
    size_t count = std::min({x.size(), y.size(), padDiameter.size(), 
                             drillDiameter.size(), netIds.size()});
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // 多线程加载
    size_t chunkSize = (count + threadCount_ - 1) / threadCount_;
    std::vector<std::future<void>> futures;
    
    for (int t = 0; t < threadCount_; ++t) {
        size_t startIdx = t * chunkSize;
        size_t endIdx = std::min(startIdx + chunkSize, count);
        
        if (startIdx < count) {
            futures.push_back(std::async(std::launch::async, [this, &x, &y, &padDiameter, &drillDiameter, &netIds, startIdx, endIdx]() {
                this->loadViasParallel(x, y, startIdx, endIdx);
            }));
        }
    }
    
    // 等待完成
    for (auto& f : futures) {
        f.wait();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Loaded " << count << " Vias in " << duration.count() << "ms" << std::endl;
}

void PCBLoader::loadViasParallel(const std::vector<double>& x,
                                const std::vector<double>& y,
                                size_t start, size_t end) {
    for (size_t i = start; i < end; ++i) {
        uint32_t viaId = model_->createVia();
        auto* via = model_->getVia(viaId);
        
        if (via) {
            via->setPosition(x[i], y[i], model_->getVariablePool());
            via->setPadDiameter(padDiameter[i], model_->getVariablePool());
            via->setDrillDiameter(drillDiameter[i], model_->getVariablePool());
            via->setNet(netIds[i]);
            
            loadedVias_++;
        }
    }
}

void PCBLoader::loadTracesBatch(const std::vector<std::vector<double>>& paths,
                               const std::vector<double>& widths,
                               const std::vector<uint32_t>& layerIds,
                               const std::vector<uint32_t>& netIds) {
    size_t count = std::min({paths.size(), widths.size(), 
                             layerIds.size(), netIds.size()});
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < count; ++i) {
        uint32_t traceId = model_->createTrace();
        auto* trace = model_->getTrace(traceId);
        
        if (trace) {
            trace->setPath(paths[i]);
            trace->setWidth(widths[i], model_->getVariablePool());
            trace->setLayer(layerIds[i]);
            trace->setNet(netIds[i]);
            
            loadedTraces_++;
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Loaded " << count << " Traces in " << duration.count() << "ms" << std::endl;
}

void PCBLoader::loadSurfacesBatch(const std::vector<std::vector<double>>& boundaries,
                                 const std::vector<std::vector<std::vector<double>>>& holes,
                                 const std::vector<uint32_t>& layerIds,
                                 const std::vector<uint32_t>& netIds) {
    size_t count = std::min({boundaries.size(), layerIds.size(), netIds.size()});
    
    for (size_t i = 0; i < count; ++i) {
        uint32_t surfId = model_->createSurface();
        auto* surf = model_->getSurface(surfId);
        
        if (surf) {
            surf->setBoundary(boundaries[i]);
            if (i < holes.size()) {
                for (const auto& hole : holes[i]) {
                    surf->addHole(hole);
                }
            }
            surf->setLayer(layerIds[i]);
            surf->setNet(netIds[i]);
            surf->calculateArea();
            
            loadedSurfaces_++;
        }
    }
}

void PCBLoader::loadComponentsBatch(ComponentType type,
                                 const std::vector<double>& x,
                                 const std::vector<double>& y,
                                 const std::vector<double>& rotation,
                                 const std::vector<uint32_t>& netIds) {
    size_t count = std::min({x.size(), y.size(), rotation.size(), netIds.size()});
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // 批量创建
    auto ids = model_->createComponents(type, static_cast<uint32_t>(count));
    
    // 批量设置属性
    for (size_t i = 0; i < count && i < ids.size(); ++i) {
        auto* comp = model_->getComponent(ids[i]);
        
        if (comp) {
            comp->setPosition(x[i], y[i], model_->getVariablePool());
            comp->setRotation(rotation[i], model_->getVariablePool());
            comp->setNet(netIds[i]);
            
            loadedComponents_++;
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Loaded " << count << " Components in " << duration.count() << "ms" << std::endl;
}

bool PCBLoader::importFromFile(const std::string& filename) {
    // 根据文件扩展名选择加载器
    if (filename.find(".gbr") != std::string::npos || 
        filename.find(".gerber") != std::string::npos) {
        // 加载Gerber
        return true;
    } else if (filename.find(".dxf") != std::string::npos) {
        return importDXF(filename);
    } else if (filename.find(".gcd") != std::string::npos) {
        return importGenCAD(filename);
    }
    
    return false;
}

bool PCBLoader::importGenCAD(const std::string& filename) {
    // 简化实现
    return false;
}

bool PCBLoader::importDXF(const std::string& filename) {
    // 简化实现
    return false;
}

// ============================================================================
// IncrementalUpdater Implementation
// ============================================================================

void IncrementalUpdater::commit() {
    auto* pool = model_->getVariablePool();
    if (!pool) return;
    
    // 批量设置变量值
    pool->setValuesBatch(dirtyParams_, paramValues_);
}

void IncrementalUpdater::refreshShapes() {
    // 刷新关联的Shape
    // 后续实现
}

// ============================================================================
// MemoryCompactor Implementation
// ============================================================================

void MemoryCompactor::compactColdData() {
    // 压缩冷数据
}

void MemoryCompactor::deduplicate() {
    // 重复数据去重
}

void MemoryCompactor::shrink() {
    // 回收空闲内存
}

double MemoryCompactor::getCompressionRatio() const {
    return 1.0;
}

} // namespace FastPCB
