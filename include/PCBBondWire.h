/**
 * @file PCBBondWire.h
 * @brief BondWire键合线 - 分层数据结构
 */

#ifndef PCBBONDWIRE_H
#define PCBBONDWIRE_H

#include "PCBConfig.h"
#include <vector>
#include <string>
#include <memory>

namespace FastPCB {

class VariablePool;

// ============================================================================
// BondWire类型
// ============================================================================

enum class BondWireType : uint8_t {
    WIRE_BOND = 0,     //  Wire Bonding（线键合）
    FLIP_CHIP = 1,    // Flip Chip（倒装片）
    TAB = 2,          // TAB键合
    WEDGE = 3         // Wedge Bonding（楔形键合）
};

enum class CurveType : uint8_t {
    STRAIGHT = 0,     // 直线
    ARC = 1,          // 圆弧
    PARABOLA = 2,     // 抛物线
    CUSTOM = 3        // 自定义曲线
};

// ============================================================================
// BondWire - 分层设计
// ============================================================================

/**
 * @brief BondWire键合线
 * 
 * 分层设计：
 * - StartPoint: 起点坐标
 * - EndPoint: 终点坐标  
 * - Curve: 曲线参数
 * - Surface: 曲面数据
 */
class PCBBondWire {
private:
    // 基础数据
    uint32_t id_;
    uint32_t label_;
    BondWireType type_;
    
    // 起点坐标（参数池索引）
    uint32_t startXIndex_, startYIndex_, startZIndex_;
    
    // 终点坐标（参数池索引）
    uint32_t endXIndex_, endYIndex_, endZIndex_;
    
    // 曲线参数
    CurveType curveType_;
    uint32_t heightIndex_;      // 弧高
    uint32_t angleIndex_;       // 角度
    uint32_t lengthIndex_;      // 长度
    
    // 物理参数
    uint32_t diameterIndex_;    // 线径
    uint32_t materialIndex_;   // 材料（铜/金/铝）
    
    // 网络
    uint32_t netID_;
    
    // 芯片/封装引用
    uint32_t sourceChipID_;    // 源芯片ID
    uint32_t targetPadID_;    // 目标焊盘ID
    
    // 几何数据
    uint32_t surfaceGeomID_;   // 曲面几何ID
    uint32_t centerlineID_;    // 中心线几何ID
    
    // 标志
    uint8_t flags_;

public:
    PCBBondWire();
    ~PCBBondWire() = default;
    
    // ==================== 创建 ====================
    
    void create(uint32_t id, uint32_t label, BondWireType type, VariablePool* pool);
    
    // ==================== 端点 ====================
    
    void setStartPosition(double x, double y, double z, VariablePool* pool);
    void setEndPosition(double x, double y, double z, VariablePool* pool);
    
    void getStartPosition(double& x, double& y, double& z, VariablePool* pool) const;
    void getEndPosition(double& x, double& y, double& z, VariablePool* pool) const;
    
    // ==================== 曲线 ====================
    
    void setCurveType(CurveType type) { curveType_ = type; }
    CurveType getCurveType() const { return curveType_; }
    
    void setHeight(double height, VariablePool* pool);
    double getHeight(VariablePool* pool) const;
    
    // ==================== 物理参数 ====================
    
    void setDiameter(double diameter, VariablePool* pool);
    double getDiameter(VariablePool* pool) const;
    
    void setMaterial(const std::string& material);
    
    // ==================== 连接 ====================
    
    void setSource(uint32_t chipID) { sourceChipID_ = chipID; }
    void setTarget(uint32_t padID) { targetPadID_ = padID; }
    
    uint32_t getSource() const { return sourceChipID_; }
    uint32_t getTarget() const { return targetPadID_; }
    
    // ==================== 网络 ====================
    
    void setNet(uint32_t netID) { netID_ = netID; }
    uint32_t getNet() const { return netID_; }
    
    // ==================== ID ====================
    
    uint32_t getID() const { return id_; }
    uint32_t getLabel() const { return label_; }
    BondWireType getType() const { return type_; }
};

// ============================================================================
// BondWire管理器
// ============================================================================

class BondWirePool {
private:
    std::vector<std::unique_ptr<PCBBondWire>> wires_;
    std::vector<uint32_t> freeList_;
    uint32_t nextID_;
    
public:
    BondWirePool();
    
    PCBBondWire* create(uint32_t label, BondWireType type, VariablePool* pool);
    PCBBondWire* get(uint32_t id);
    void remove(uint32_t id);
    
    std::vector<PCBBondWire*> getByNet(uint32_t netID);
    std::vector<PCBBondWire*> getByChip(uint32_t chipID);
    
    size_t size() const { return wires_.size(); }
};

} // namespace FastPCB

#endif // PCBBONDWIRE_H
