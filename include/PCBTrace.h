/**
 * @file PCBTrace.h
 * @brief PCB走线 - 分层数据结构
 */

#ifndef PCBTRACE_H
#define PCBTRACE_H

#include "PCBConfig.h"
#include <vector>
#include <memory>

namespace FastPCB {

class VariablePool;

// ============================================================================
// PCB走线 - 分层设计
// ============================================================================

/**
 * @brief PCB走线（Trace）
 * 
 * 分层设计：
 * - Path: 路径几何
 * - Width: 线宽参数
 * - Layer: 层引用
 * - Net: 网络引用
 */
class PCBTrace {
private:
    // 基础数据
    uint32_t id_;
    uint32_t label_;
    
    // 路径点（紧凑数组存储）
    std::vector<double> pathPoints_;  // [x1, y1, x2, y2, ...]
    
    // 线宽参数（参数池索引）
    uint32_t widthIndex_;
    
    // 层信息（整数ID）
    uint32_t layerID_;
    
    // 网络
    uint32_t netID_;
    
    // 几何缓存
    uint32_t geometryID_;
    
    // 标志
    uint8_t flags_;  // 0x01: 差分对, 0x02: 关键信号, 0x04: 受控阻抗
    
public:
    PCBTrace();
    ~PCBTrace() = default;
    
    // ==================== 创建 ====================
    
    void create(uint32_t id, uint32_t label, VariablePool* pool);
    
    // ==================== 路径 ====================
    
    /**
     * @brief 设置路径点
     */
    void setPath(const std::vector<double>& points);
    
    /**
     * @brief 添加路径点
     */
    void addPoint(double x, double y);
    
    /**
     * @brief 获取路径点
     */
    const std::vector<double>& getPath() const { return pathPoints_; }
    
    /**
     * @brief 获取点数量
     */
    size_t getPointCount() const { return pathPoints_.size() / 2; }
    
    // ==================== 线宽 ====================
    
    void setWidth(double width, VariablePool* pool);
    double getWidth(VariablePool* pool) const;
    
    // ==================== 层 ====================
    
    void setLayer(uint32_t layerID) { layerID_ = layerID; }
    uint32_t getLayer() const { return layerID_; }
    
    // ==================== 网络 ====================
    
    void setNet(uint32_t netID) { netID_ = netID; }
    uint32_t getNet() const { return netID_; }
    
    // ==================== ID ====================
    
    uint32_t getID() const { return id_; }
    uint32_t getLabel() const { return label_; }
    
    // ==================== 几何 ====================
    
    void setGeometryID(uint32_t id) { geometryID_ = id; }
    uint32_t getGeometryID() const { return geometryID_; }
    
    // ==================== 标志 ====================
    
    bool isDifferential() const { return flags_ & 0x01; }
    void setDifferential(bool diff) { if (diff) flags_ |= 0x01; else flags_ &= ~0x01; }
    
    bool isCriticalSignal() const { return flags_ & 0x02; }
    void setCriticalSignal(bool critical) { if (critical) flags_ |= 0x02; else flags_ &= ~0x02; }
    
    bool isControlledImpedance() const { return flags_ & 0x04; }
    void setControlledImpedance(bool ci) { if (ci) flags_ |= 0x04; else flags_ &= ~0x04; }
};

// ============================================================================
// 走线管理器
// ============================================================================

class TracePool {
private:
    std::vector<std::unique_ptr<PCBTrace>> traces_;
    std::vector<uint32_t> freeList_;
    uint32_t nextID_;
    
    // 网络索引
    std::unordered_map<uint32_t, std::vector<uint32_t>> netIndex_;
    // 层索引
    std::unordered_map<uint32_t, std::vector<uint32_t>> layerIndex_;
    
public:
    TracePool();
    
    PCBTrace* create(uint32_t label, VariablePool* pool);
    PCBTrace* get(uint32_t id);
    void remove(uint32_t id);
    
    std::vector<PCBTrace*> getByNet(uint32_t netID);
    std::vector<PCBTrace*> getByLayer(uint32_t layerID);
    
    size_t size() const { return traces_.size(); }
};

} // namespace FastPCB
