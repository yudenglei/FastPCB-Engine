/**
 * @file PCBDataModel.cpp
 * @brief PCB数据模型实现
 */

#include "PCBDataModel.h"
#include <iostream>
#include <thread>
#include <future>

namespace FastPCB {

// ============================================================================
// PCBDataModel Implementation
// ============================================================================

PCBDataModel::PCBDataModel() 
    : totalMemory_(0),
      componentCount_(0),
      viaCount_(0),
      traceCount_(0),
      surfaceCount_(0) {
}

PCBDataModel::~PCBDataModel() {
}

bool PCBDataModel::initialize() {
    try {
        // 初始化OCAF
        createOCAFStructure();
        
        // 初始化变量池
        variablePool_ = std::make_unique<VariablePool>();
        refPool_ = std::make_unique<RefPool>();
        
        // 初始化对象池
        componentPool_ = std::make_unique<ComponentPool>(variablePool_.get(), 1000000);
        viaPool_ = std::make_unique<ViaPool>();
        bondWirePool_ = std::make_unique<BondWirePool>();
        tracePool_ = std::make_unique<TracePool>();
        surfacePool_ = std::make_unique<SurfacePool>();
        
        // 创建Label预分配
        createLabelAllocation();
        
        std::cout << "PCB Data Model initialized successfully." << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize PCB Data Model: " << e.what() << std::endl;
        return false;
    }
}

void PCBDataModel::createOCAFStructure() {
    // 创建OCAF文档
    Handle(TDocStd_Document) doc = new TDocStd_Document("FastPCB");
    document_ = doc;
    
    // 获取根Label
    root_ = doc->Main();
    
    // 创建变量池根（Label 101-1000）
    variableRoot_ = root_.NewChild();
    variableRoot_.SetLabel("Variables");
    
    // 创建层叠结构根
    layerStackRoot_ = root_.NewChild();
    layerStackRoot_.SetLabel("LayerStack");
    
    // 创建网络根
    networkRoot_ = root_.NewChild();
    networkRoot_.SetLabel("Networks");
}

void PCBDataModel::createLabelAllocation() {
    // 预分配Label结构
    // 这里可以添加Label预创建逻辑
    
    std::cout << "Label allocation created (variables at 101-1000)" << std::endl;
}

bool PCBDataModel::load(const std::string& filename) {
    // 后续实现文件加载
    return false;
}

bool PCBDataModel::save(const std::string& filename) {
    // 后续实现文件保存
    return false;
}

// ==================== 变量池 ====================

uint32_t PCBDataModel::createVariable(const std::string& name, double value) {
    return variablePool_->createOrGet(name, value);
}

void PCBDataModel::setVariable(uint32_t index, double value) {
    variablePool_->setValue(index, value);
}

double PCBDataModel::getVariable(uint32_t index) const {
    return variablePool_->getValue(index);
}

void PCBDataModel::setExpression(uint32_t index, const std::string& expr) {
    variablePool_->setExpression(index, expr);
}

// ==================== 器件 ====================

uint32_t PCBDataModel::createComponent(ComponentType type) {
    uint32_t id = componentPool_->create(type, componentCount_.load() + LabelConfig::COMPONENTS_START);
    componentCount_++;
    return id;
}

PCBComponent* PCBDataModel::getComponent(uint32_t id) {
    return componentPool_->get(id);
}

void PCBDataModel::removeComponent(uint32_t id) {
    componentPool_->remove(id);
    componentCount_--;
}

std::vector<uint32_t> PCBDataModel::createComponents(ComponentType type, uint32_t count) {
    uint32_t startLabel = componentCount_.load() + LabelConfig::COMPONENTS_START;
    auto ids = componentPool_->createBatch(type, count, startLabel);
    componentCount_ += count;
    return ids;
}

// ==================== Via ====================

uint32_t PCBDataModel::createVia() {
    uint32_t id = viaPool_->create(viaCount_.load() + LabelConfig::VIA_RANGE_START, variablePool_.get());
    viaCount_++;
    return id;
}

PCBVia* PCBDataModel::getVia(uint32_t id) {
    return viaPool_->get(id);
}

// ==================== BondWire ====================

uint32_t PCBDataModel::createBondWire(BondWireType type) {
    uint32_t id = bondWirePool_->create(bondWirePool_.size() + LabelConfig::BONDWIRE_RANGE_START, type, variablePool_.get());
    return id;
}

PCBBondWire* PCBDataModel::getBondWire(uint32_t id) {
    return bondWirePool_->get(id);
}

// ==================== Trace ====================

uint32_t PCBDataModel::createTrace() {
    uint32_t id = tracePool_->create(traceCount_.load() + LabelConfig::TRACE_RANGE_START, variablePool_.get());
    traceCount_++;
    return id;
}

PCBTrace* PCBDataModel::getTrace(uint32_t id) {
    return tracePool_->get(id);
}

// ==================== Surface ====================

uint32_t PCBDataModel::createSurface() {
    uint32_t id = surfacePool_->create(surfaceCount_.load() + LabelConfig::SURFACE_RANGE_START, variablePool_.get());
    surfaceCount_++;
    return id;
}

PCBSurface* PCBDataModel::getSurface(uint32_t id) {
    return surfacePool_->get(id);
}

// ==================== 网络 ====================

uint32_t PCBDataModel::createNet(const std::string& name) {
    auto it = netNameToID_.find(name);
    if (it != netNameToID_.end()) {
        return it->second;
    }
    
    uint32_t id = netNameToID_.size() + 1;
    netNameToID_[name] = id;
    return id;
}

uint32_t PCBDataModel::getNetID(const std::string& name) const {
    auto it = netNameToID_.find(name);
    if (it != netNameToID_.end()) {
        return it->second;
    }
    return 0;
}

// ==================== 统计 ====================

uint64_t PCBDataModel::getEstimatedMemory() const {
    uint64_t memory = 0;
    
    // 器件
    memory += componentCount_.load() * sizeof(PCBComponent);
    
    // Via
    memory += viaCount_.load() * sizeof(PCBVia);
    
    // Trace
    memory += traceCount_.load() * sizeof(PCBTrace);
    
    // Surface
    memory += surfaceCount_.load() * sizeof(PCBSurface);
    
    // 变量池
    if (variablePool_) {
        memory += variablePool_->capacity() * sizeof(Variable);
    }
    
    return memory;
}

} // namespace FastPCB
