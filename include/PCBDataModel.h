/**
 * @file PCBDataModel.h
 * @brief PCB数据模型 - 核心：OCAF组织+分层管理
 */

#ifndef PCBDATAMODEL_H
#define PCBDATAMODEL_H

#include "PCBConfig.h"
#include "PCBMemoryPool.h"
#include "PCBVariablePool.h"
#include "PCBRefPool.h"
#include "PCBComponent.h"
#include "PCBVia.h"
#include "PCBBondWire.h"
#include "PCBTrace.h"
#include "PCBSurface.h"

#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>

// OpenCASCADE
#include <TDocStd_Document.hxx>
#include <TDF_Label.hxx>
#include <TDF_LabelSequence.hxx>
#include <Handle.hxx>

namespace FastPCB {

// ============================================================================
// PCB数据模型 - 核心类
// ============================================================================

/**
 * @brief PCB数据模型
 * 
 * 组织结构：
 * - OCAF文档作为根
 * - 分层数据管理
 * - 参数化变量池前置
 * 
 * 优化策略：
 * - 池化数据前置（Label 101-1000）
 * - 紧凑数据结构
 * - 延迟加载
 */
class PCBDataModel {
private:
    // OCAF文档
    Handle(TDocStd_Document) document_;
    TDF_Label root_;
    
    // 变量池（前置！重要优化）
    std::unique_ptr<VariablePool> variablePool_;
    TDF_Label variableRoot_;
    
    // Reference池
    std::unique_ptr<RefPool> refPool_;
    
    // 器件管理
    std::unique_ptr<ComponentPool> componentPool_;
    
    // 几何对象管理
    std::unique_ptr<ViaPool> viaPool_;
    std::unique_ptr<BondWirePool> bondWirePool_;
    std::unique_ptr<TracePool> tracePool_;
    std::unique_ptr<SurfacePool> surfacePool_;
    
    // 层叠结构
    TDF_Label layerStackRoot_;
    std::vector<uint32_t> layerIDs_;
    
    // 网络
    TDF_Label networkRoot_;
    std::unordered_map<std::string, uint32_t> netNameToID_;
    
    // 统计
    std::atomic<uint64_t> totalMemory_;
    std::atomic<uint32_t> componentCount_;
    std::atomic<uint32_t> viaCount_;
    std::atomic<uint32_t> traceCount_;
    std::atomic<uint32_t> surfaceCount_;
    
    // 互斥
    std::mutex mutex_;
    
public:
    PCBDataModel();
    ~PCBDataModel();
    
    // ==================== 初始化 ====================
    
    /**
     * @brief 初始化模型
     */
    bool initialize();
    
    /**
     * @brief 加载文件
     */
    bool load(const std::string& filename);
    
    /**
     * @brief 保存文件
     */
    bool save(const std::string& filename);
    
    // ==================== 变量池 ====================
    
    /**
     * @brief 创建变量
     */
    uint32_t createVariable(const std::string& name, double value = 0.0);
    
    /**
     * @brief 设置变量值
     */
    void setVariable(uint32_t index, double value);
    
    /**
     * @brief 获取变量值
     */
    double getVariable(uint32_t index) const;
    
    /**
     * @brief 设置表达式
     */
    void setExpression(uint32_t index, const std::string& expr);
    
    VariablePool* getVariablePool() { return variablePool_.get(); }
    
    // ==================== 器件 ====================
    
    /**
     * @brief 创建器件
     */
    uint32_t createComponent(ComponentType type);
    
    /**
     * @brief 获取器件
     */
    PCBComponent* getComponent(uint32_t id);
    
    /**
     * @brief 删除器件
     */
    void removeComponent(uint32_t id);
    
    /**
     * @brief 批量创建器件
     */
    std::vector<uint32_t> createComponents(ComponentType type, uint32_t count);
    
    // ==================== Via ====================
    
    /**
     * @brief 创建过孔
     */
    uint32_t createVia();
    
    /**
     * @brief 获取过孔
     */
    PCBVia* getVia(uint32_t id);
    
    // ==================== BondWire ====================
    
    /**
     * @brief 创建键合线
     */
    uint32_t createBondWire(BondWireType type);
    
    /**
     * @brief 获取键合线
     */
    PCBBondWire* getBondWire(uint32_t id);
    
    // ==================== Trace ====================
    
    /**
     * @brief 创建走线
     */
    uint32_t createTrace();
    
    /**
     * @brief 获取走线
     */
    PCBTrace* getTrace(uint32_t id);
    
    // ==================== Surface ====================
    
    /**
     * @brief 创建铜皮
     */
    uint32_t createSurface();
    
    /**
     * @brief 获取铜皮
     */
    PCBSurface* getSurface(uint32_t id);
    
    // ==================== 网络 ====================
    
    /**
     * @brief 创建网络
     */
    uint32_t createNet(const std::string& name);
    
    /**
     * @brief 获取网络ID
     */
    uint32_t getNetID(const std::string& name) const;
    
    // ==================== 统计 ====================
    
    uint32_t getComponentCount() const { return componentCount_.load(); }
    uint32_t getViaCount() const { return viaCount_.load(); }
    uint32_t getTraceCount() const { return traceCount_.load(); }
    uint32_t getSurfaceCount() const { return surfaceCount_.load(); }
    
    /**
     * @brief 获取内存占用（估算）
     */
    uint64_t getEstimatedMemory() const;
    
    // ==================== OCAF访问 ====================
    
    Handle(TDocStd_Document) getDocument() { return document_; }
    TDF_Label getRoot() { return root_; }
    TDF_Label getVariableRoot() { return variableRoot_; }
    
private:
    /**
     * @brief 创建OCAF结构
     */
    void createOCAFStructure();
    
    /**
     * @brief 创建Label预分配
     */
    void createLabelAllocation();
};

} // namespace FastPCB

#endif // PCBDATAMODEL_H
