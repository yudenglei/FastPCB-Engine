/**
 * @file PCBVia.h
 * @brief PCB过孔 - 分层数据结构
 */

#ifndef PCBVIA_H
#define PCBVIA_H

#include "PCBConfig.h"
#include "PCBRefPool.h"
#include <vector>
#include <memory>

namespace FastPCB {

class VariablePool;

// ============================================================================
// PCB过孔 - 分层设计
// ============================================================================

/**
 * @brief PCB过孔（分层数据结构）
 * 
 * 分层设计：
 * - PadTop: 顶层铜皮
 * - PadBottom: 底层铜皮  
 * - Drill: 钻孔
 * - Parameters: 参数化变量
 * 
 * 内存优化：
 * - 使用参数池索引代替Label
 * - Reference使用整数ID
 */
class PCBVia {
private:
    // 基础数据
    uint32_t id_;
    uint32_t label_;
    
    // 坐标参数（使用参数池索引 - 紧凑）
    uint32_t xIndex_;           // X坐标参数池索引
    uint32_t yIndex_;           // Y坐标参数池索引
    
    // 尺寸参数
    uint32_t padDiameterIndex_;    // 焊盘直径
    uint32_t drillDiameterIndex_;  // 钻孔直径
    
    // 层信息（整数ID代替Label）
    uint32_t topLayerID_;
    uint32_t bottomLayerID_;
    uint32_t startLayerID_;
    uint32_t endLayerID_;
    
    // 网络（整数ID）
    uint32_t netID_;
    
    // 几何数据（分层存储）
    uint32_t padTopGeomID_;    // 顶层焊盘几何ID
    uint32_t padBotGeomID_;    // 底层焊盘几何ID
    uint32_t drillGeomID_;     // 钻孔几何ID
    
    // 标志
    uint8_t flags_;
    uint8_t padCount_;         // 焊盘数量（thermal, anti-pad等）

public:
    PCBVia();
    ~PCBVia() = default;
    
    // ==================== 创建 ====================
    
    /**
     * @brief 创建过孔
     */
    void create(uint32_t id, uint32_t label, VariablePool* pool);
    
    // ==================== 位置 ====================
    
    /**
     * @brief 设置位置
     */
    void setPosition(double x, double y, VariablePool* pool);
    
    /**
     * @brief 获取位置
     */
    void getPosition(double& x, double& y, VariablePool* pool) const;
    
    // ==================== 尺寸 ====================
    
    /**
     * @brief 设置焊盘直径
     */
    void setPadDiameter(double diameter, VariablePool* pool);
    double getPadDiameter(VariablePool* pool) const;
    
    /**
     * @brief 设置钻孔直径
     */
    void setDrillDiameter(double diameter, VariablePool* pool);
    double getDrillDiameter(VariablePool* pool) const;
    
    // ==================== 层 ====================
    
    /**
     * @brief 设置起止层
     */
    void setLayers(uint32_t startLayer, uint32_t endLayer);
    void getLayers(uint32_t& startLayer, uint32_t& endLayer) const;
    
    // ==================== 网络 ====================
    
    /**
     * @brief 设置网络
     */
    void setNet(uint32_t netID);
    uint32_t getNet() const { return netID_; }
    
    // ==================== ID ====================
    
    uint32_t getID() const { return id_; }
    uint32_t getLabel() const { return label_; }
    
    // ==================== 几何 ====================
    
    void setPadTopGeomID(uint32_t id) { padTopGeomID_ = id; }
    void setPadBottomGeomID(uint32_t id) { padBotGeomID_ = id; }
    void setDrillGeomID(uint32_t id) { drillGeomID_ = id; }
    
    uint32_t getPadTopGeomID() const { return padTopGeomID_; }
    uint32_t getPadBottomGeomID() const { return padBotGeomID_; }
    uint32_t getDrillGeomID() const { return drillGeomID_; }
    
    // ==================== 标志 ====================
    
    bool isPlated() const { return flags_ & 0x01; }
    void setPlated(bool plated) { if (plated) flags_ |= 0x01; else flags_ &= ~0x01; }
    
    bool isTented() const { return flags_ & 0x02; }
    void setTented(bool tented) { if (tented) flags_ |= 0x02; else flags_ &= ~0x02; }
};

// ============================================================================
// 过孔管理器 - 对象池
// ============================================================================

class ViaPool {
private:
    std::vector<std::unique_ptr<PCBVia>> vias_;
    std::vector<uint32_t> freeList_;
    uint32_t nextID_;
    
public:
    ViaPool();
    
    /**
     * @brief 创建过孔
     */
    PCBVia* create(uint32_t label, VariablePool* pool);
    
    /**
     * @brief 获取过孔
     */
    PCBVia* get(uint32_t id);
    
    /**
     * @brief 删除过孔
     */
    void remove(uint32_t id);
    
    /**
     * @brief 批量创建
     */
    std::vector<PCBVia*> createBatch(uint32_t count, uint32_t startLabel, VariablePool* pool);
    
    size_t size() const { return vias_.size(); }
};

} // namespace FastPCB

#endif // PCBVIA_H
