/**
 * @file PCBSurface.h
 * @brief PCB铜皮 - 分层数据结构
 */

#ifndef PCBSURFACE_H
#define PCBSURFACE_H

#include "PCBConfig.h"
#include <vector>
#include <memory>
#include <unordered_map>

namespace FastPCB {

class VariablePool;

// ============================================================================
// PCB铜皮 - 分层设计
// ============================================================================

/**
 * @brief PCB铜皮（Surface）
 * 
 * 分层设计：
 * - Boundary: 边界多边形
 * - Holes: 挖孔列表
 * - Net: 网络引用
 * - Layer: 层引用
 */
class PCBSurface {
private:
    // 基础数据
    uint32_t id_;
    uint32_t label_;
    
    // 边界多边形
    std::vector<double> boundary_;  // [x1, y1, x2, y2, ...]
    
    // 挖孔列表
    std::vector<std::vector<double>> holes_;  // 多个多边形
    
    // 层信息
    uint32_t layerID_;
    
    // 网络
    uint32_t netID_;
    
    // 几何数据
    uint32_t geometryID_;
    
    // 属性
    uint8_t type_;  // 0: Plane, 1: Signal, 2: Mask, 3: Paste
    uint8_t flags_;
    
    // 面积（缓存）
    double area_;
    
public:
    PCBSurface();
    ~PCBSurface() = default;
    
    // ==================== 创建 ====================
    
    void create(uint32_t id, uint32_t label, VariablePool* pool);
    
    // ==================== 边界 ====================
    
    /**
     * @brief 设置边界
     */
    void setBoundary(const std::vector<double>& points);
    
    /**
     * @brief 添加边界点
     */
    void addBoundaryPoint(double x, double y);
    
    /**
     * @brief 获取边界
     */
    const std::vector<double>& getBoundary() const { return boundary_; }
    
    // ==================== 挖孔 ====================
    
    /**
     * @brief 添加挖孔
     */
    void addHole(const std::vector<double>& points);
    
    /**
     * @brief 移除挖孔
     */
    void removeHole(size_t index);
    
    /**
     * @brief 获取挖孔数量
     */
    size_t getHoleCount() const { return holes_.size(); }
    
    /**
     * @brief 获取挖孔
     */
    const std::vector<double>& getHole(size_t index) const { return holes_[index]; }
    
    // ==================== 层 ====================
    
    void setLayer(uint32_t layerID) { layerID_ = layerID; }
    uint32_t getLayer() const { return layerID_; }
    
    // ==================== 网络 ====================
    
    void setNet(uint32_t netID) { netID_ = netID; }
    uint32_t getNet() const { return netID_; }
    
    // ==================== ID ====================
    
    uint32_t getID() const { return id_; }
    uint32_t getLabel() const { return label_; }
    
    // ==================== 类型 ====================
    
    void setType(uint8_t type) { type_ = type; }
    uint8_t getType() const { return type_; }
    
    // ==================== 几何 ====================
    
    void setGeometryID(uint32_t id) { geometryID_ = id; }
    uint32_t getGeometryID() const { return geometryID_; }
    
    // ==================== 面积 ====================
    
    void calculateArea();
    double getArea() const { return area_; }
};

// ============================================================================
// 铜皮管理器
// ============================================================================

class SurfacePool {
private:
    std::vector<std::unique_ptr<PCBSurface>> surfaces_;
    std::vector<uint32_t> freeList_;
    uint32_t nextID_;
    
    // 索引
    std::unordered_map<uint32_t, std::vector<uint32_t>> netIndex_;
    std::unordered_map<uint32_t, std::vector<uint32_t>> layerIndex_;
    
public:
    SurfacePool();
    
    PCBSurface* create(uint32_t label, VariablePool* pool);
    PCBSurface* get(uint32_t id);
    void remove(uint32_t id);
    
    std::vector<PCBSurface*> getByNet(uint32_t netID);
    std::vector<PCBSurface*> getByLayer(uint32_t layerID);
    
    size_t size() const { return surfaces_.size(); }
};

} // namespace FastPCB

#endif // PCBSURFACE_H
