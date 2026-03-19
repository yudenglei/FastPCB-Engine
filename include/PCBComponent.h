/**
 * @file PCBComponent.h
 * @brief PCB器件基类 - 内存优化：紧凑结构+延迟加载
 */

#ifndef PCBCOMPONENT_H
#define PCBCOMPONENT_H

#include "PCBConfig.h"
#include "PCBMemoryPool.h"
#include "PCBRefPool.h"
#include <vector>
#include <string>
#include <memory>
#include <atomic>

namespace FastPCB {

// 前向声明
class VariablePool;
class PCBDataModel;

// ============================================================================
// PCB器件基类 - 紧凑结构优化
// ============================================================================

/**
 * @brief PCB器件基类
 * 
 * 内存优化策略：
 * 1. 使用紧凑结构体而非类
 * 2. 使用整数ID代替Label引用
 * 3. 延迟加载非必要数据
 * 4. 参数使用紧凑数组存储
 */
struct PCBComponent {
    // -------------------- 基础数据（紧凑存储）--------------------
    uint32_t id_;                    // 器件ID（4字节）
    uint32_t label_;                 // OCAF Label（4字节）
    ComponentType type_;             // 器件类型（1字节）
    uint8_t flags_;                  // 标志（1字节）
    uint8_t padding_[2];            // 对齐填充（2字节）
    
    // -------------------- 坐标参数（紧凑数组）--------------------
    // 优化：使用参数池索引而非Label
    uint32_t xIndex_;               // X坐标参数池索引
    uint32_t yIndex_;               // Y坐标参数池索引
    uint32_t rotationIndex_;         // 旋转参数池索引
    
    // -------------------- 引用（整数ID代替Label）--------------------
    uint32_t netID_;                // 网络ID（整数代替Reference）
    uint32_t packageID_;            // 封装ID
    uint32_t symbolID_;             // 符号ID
    
    // -------------------- 扩展数据（延迟加载）--------------------
    uint32_t extDataOffset_;        // 扩展数据偏移量（延迟加载）
    uint32_t geometryCount_;        // 几何对象数量
    
    // -------------------- 统计--------------------
    std::atomic<uint32_t> refCount_;    // 被引用次数
    std::atomic<uint32_t> accessCount_; // 访问次数（LRU用）
    
public:
    PCBComponent() 
        : id_(0), label_(0), type_(ComponentType::IC), flags_(0),
          xIndex_(0), yIndex_(0), rotationIndex_(0),
          netID_(0), packageID_(0), symbolID_(0),
          extDataOffset_(0), geometryCount_(0),
          refCount_(0), accessCount_(0) {}
    
    // 设置位置
    void setPosition(double x, double y, VariablePool* pool);
    void getPosition(double& x, double& y, VariablePool* pool) const;
    
    // 设置旋转
    void setRotation(double rotation, VariablePool* pool);
    double getRotation(VariablePool* pool) const;
    
    // 网络操作
    void setNet(uint32_t netId) { netID_ = netId; }
    uint32_t getNet() const { return netID_; }
    
    // 引用计数
    void incRef() { refCount_++; }
    void decRef() { if (refCount_ > 0) refCount_--; }
    uint32_t getRefCount() const { return refCount_; }
    
    // 访问统计
    void recordAccess() { accessCount_++; }
    uint32_t getAccessCount() const { return accessCount_; }
    
    // 标志操作
    bool isVisible() const { return flags_ & 0x01; }
    void setVisible(bool v) { if (v) flags_ |= 0x01; else flags_ &= ~0x01; }
    bool isSelected() const { return flags_ & 0x02; }
    void setSelected(bool v) { if (v) flags_ |= 0x02; else flags_ &= ~0x02; }
    bool isLocked() const { return flags_ & 0x04; }
    void setLocked(bool v) { if (v) flags_ |= 0x04; else flags_ &= ~0x04; }
};

// ============================================================================
// 器件管理器 - 对象池+批量操作
// ============================================================================

class ComponentPool {
private:
    // 预分配的器件数组（核心优化）
    std::vector<PCBComponent> components_;
    std::vector<uint32_t> freeList_;
    std::atomic<uint32_t> nextId_;
    
    // 快速索引：Label -> Index
    std::vector<int32_t> labelToIndex_;
    
    // 类型索引
    std::vector<std::vector<uint32_t>> typeIndices_;
    
    // 网络索引：NetID -> ComponentIDs
    std::unordered_map<uint32_t, std::vector<uint32_t>> netIndex_;
    
    VariablePool* varPool_;
    std::mutex mutex_;
    
public:
    explicit ComponentPool(VariablePool* pool, size_t maxComponents = 1000000);
    
    // 创建器件
    uint32_t create(ComponentType type, uint32_t label);
    
    // 获取器件
    PCBComponent* get(uint32_t id);
    PCBComponent* getByLabel(uint32_t label);
    
    // 删除器件
    void remove(uint32_t id);
    
    // 批量操作（优化）
    std::vector<PCBComponent*> getByNet(uint32_t netId);
    std::vector<PCBComponent*> getByType(ComponentType type);
    
    // 批量创建
    std::vector<uint32_t> createBatch(ComponentType type, uint32_t count, uint32_t startLabel);
    
    // 批量删除
    void removeBatch(const std::vector<uint32_t>& ids);
    
    // 更新索引
    void updateIndices(uint32_t id);
    
    size_t size() const { return nextId_.load(); }
    size_t capacity() const { return components_.size(); }

private:
    void buildIndex(uint32_t id);
    void removeFromIndex(uint32_t id);
};

// ============================================================================
// 器件工厂 - 简单工厂模式
// ============================================================================

class ComponentFactory {
public:
    static std::unique_ptr<PCBComponent> create(ComponentType type);
    
    static ComponentType stringToType(const std::string& str);
    static std::string typeToString(ComponentType type);
};

} // namespace FastPCB

#endif // PCBCOMPONENT_H
