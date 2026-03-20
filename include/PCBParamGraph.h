/**
 * @file PCBParamGraph.h
 * @brief 参数化变量依赖图 - 有向图实现
 */

#ifndef PCBPARAMGRAPH_H
#define PCBPARAMGRAPH_H

#include "PCBConfig.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <stack>
#include <queue>
#include <functional>

namespace FastPCB {

// ============================================================================
// 参数化变量节点
// ============================================================================

/**
 * @brief 参数化变量节点（有向图）
 * 
 * 使用有向无环图（DAG）管理参数依赖关系
 */
class ParamNode {
private:
    uint32_t id_;                    // 节点ID
    std::string name_;               // 变量名
    double value_;                  // 当前值
    double cachedValue_;             // 缓存的计算值
    bool isDirty_;                   // 是否需要重新计算
    bool isComputing_;               // 正在计算中（检测循环依赖）
    
    // 有向图边
    std::vector<uint32_t> dependents_;   // 依赖此节点的节点（下游）
    std::vector<uint32_t> dependencies_; // 此节点依赖的节点（上游）
    
public:
    ParamNode(uint32_t id, const std::string& name, double value = 0.0)
        : id_(id), name_(name), value_(value), cachedValue_(value),
          isDirty_(false), isComputing_(false) {}
    
    // 节点操作
    uint32_t getID() const { return id_; }
    const std::string& getName() const { return name_; }
    double getValue() const { return value_; }
    double getCachedValue() const { return cachedValue_; }
    
    void setValue(double v) { 
        if (value_ != v) {
            value_ = v;
            isDirty_ = true;
        }
    }
    
    void setCachedValue(double v) { cachedValue_ = v; }
    void markDirty() { isDirty_ = true; }
    void clearDirty() { isDirty_ = false; }
    bool isDirty() const { return isDirty_; }
    
    void setComputing(bool computing) { isComputing_ = computing; }
    bool isComputing() const { return isComputing_; }
    
    // 图操作
    void addDependent(uint32_t nodeId) {
        dependents_.push_back(nodeId);
    }
    
    void addDependency(uint32_t nodeId) {
        dependencies_.push_back(nodeId);
    }
    
    const std::vector<uint32_t>& getDependents() const { return dependents_; }
    const std::vector<uint32_t>& getDependencies() const { return dependencies_; }
    
    // 移除依赖关系
    void removeDependent(uint32_t nodeId) {
        dependents_.erase(std::remove(dependents_.begin(), dependents_.end(), nodeId), dependents_.end());
    }
    
    void removeDependency(uint32_t nodeId) {
        dependencies_.erase(std::remove(dependencies_.begin(), dependencies_.end(), nodeId), dependencies_.end());
    }
};

// ============================================================================
// 参数化变量依赖图（有向无环图DAG）
// ============================================================================

/**
 * @brief 参数化变量依赖图
 * 
 * 使用DAG管理参数化变量的依赖关系
 * - 拓扑排序确定计算顺序
 * - 循环检测
 * - 增量更新
 */
class ParamGraph {
private:
    std::unordered_map<uint32_t, std::unique_ptr<ParamNode>> nodes_;
    std::unordered_map<std::string, uint32_t> nameToId_;
    uint32_t nextId_;
    
    // 拓扑排序缓存
    std::vector<uint32_t> topologicalOrder_;
    bool isTopoValid_;
    
public:
    ParamGraph() : nextId_(0), isTopoValid_(false) {}
    
    // ==================== 节点管理 ====================
    
    /**
     * @brief 创建变量节点
     */
    uint32_t createVariable(const std::string& name, double value = 0.0) {
        // 检查是否已存在
        auto it = nameToId_.find(name);
        if (it != nameToId_.end()) {
            return it->second;  // 返回已有节点
        }
        
        uint32_t id = nextId_++;
        nodes_[id] = std::make_unique<ParamNode>(id, name, value);
        nameToId_[name] = id;
        
        isTopoValid_ = false;
        return id;
    }
    
    /**
     * @brief 获取节点
     */
    ParamNode* getNode(uint32_t id) {
        auto it = nodes_.find(id);
        return it != nodes_.end() ? it->second.get() : nullptr;
    }
    
    /**
     * @brief 通过名称获取节点
     */
    ParamNode* getNode(const std::string& name) {
        auto it = nameToId_.find(name);
        if (it != nameToId_.end()) {
            return getNode(it->second);
        }
        return nullptr;
    }
    
    // ==================== 依赖关系 ====================
    
    /**
     * @brief 添加依赖关系: dependent depends on dependency
     * 即: dependent -> dependency (dependent依赖于dependency)
     * 
     * @param dependent 依赖方（下游）
     * @param dependency 被依赖方（上游）
     */
    void addDependency(uint32_t dependent, uint32_t dependency) {
        auto* depNode = getNode(dependent);
        auto* refNode = getNode(dependency);
        
        if (depNode && refNode) {
            depNode->addDependency(dependency);
            refNode->addDependent(dependent);
            isTopoValid_ = false;
        }
    }
    
    /**
     * @brief 通过名称添加依赖
     */
    void addDependency(const std::string& dependent, const std::string& dependency) {
        uint32_t depId = createVariable(dependent);
        uint32_t refId = createVariable(dependency);
        addDependency(depId, refId);
    }
    
    /**
     * @brief 移除依赖关系
     */
    void removeDependency(uint32_t dependent, uint32_t dependency) {
        auto* depNode = getNode(dependent);
        auto* refNode = getNode(dependency);
        
        if (depNode && refNode) {
            depNode->removeDependency(dependency);
            refNode->removeDependent(dependent);
            isTopoValid_ = false;
        }
    }
    
    // ==================== 依赖分析 ====================
    
    /**
     * @brief 检测循环依赖
     */
    bool hasCycle() const {
        std::unordered_set<uint32_t> visiting;
        std::unordered_set<uint32_t> visited;
        
        for (const auto& pair : nodes_) {
            if (detectCycleDFS(pair.first, visiting, visited)) {
                return true;
            }
        }
        return false;
    }
    
    /**
     * @brief 获取拓扑排序
     */
    std::vector<uint32_t> getTopologicalOrder() {
        if (isTopoValid_) {
            return topologicalOrder_;
        }
        
        topologicalOrder_.clear();
        
        // Kahn算法
        std::unordered_map<uint32_t, int> inDegree;
        for (const auto& pair : nodes_) {
            inDegree[pair.first] = 0;
        }
        
        // 计算入度
        for (const auto& pair : nodes_) {
            for (uint32_t dep : pair.second->getDependencies()) {
                inDegree[pair.first]++;
            }
        }
        
        // 入度为0的节点入队
        std::queue<uint32_t> q;
        for (const auto& pair : inDegree) {
            if (pair.second == 0) {
                q.push(pair.first);
            }
        }
        
        while (!q.empty()) {
            uint32_t nodeId = q.front();
            q.pop();
            topologicalOrder_.push_back(nodeId);
            
            // 更新依赖节点的入度
            auto* node = getNode(nodeId);
            if (node) {
                for (uint32_t dependent : node->getDependents()) {
                    inDegree[dependent]--;
                    if (inDegree[dependent] == 0) {
                        q.push(dependent);
                    }
                }
            }
        }
        
        isTopoValid_ = true;
        return topologicalOrder_;
    }
    
    /**
     * @brief 获取受影响的节点（下游所有节点）
     */
    std::vector<uint32_t> getAffectedNodes(uint32_t nodeId) {
        std::vector<uint32_t> affected;
        std::unordered_set<uint32_t> visited;
        
        std::queue<uint32_t> q;
        q.push(nodeId);
        visited.insert(nodeId);
        
        while (!q.empty()) {
            uint32_t current = q.front();
            q.pop();
            
            auto* node = getNode(current);
            if (node) {
                for (uint32_t dependent : node->getDependents()) {
                    if (visited.find(dependent) == visited.end()) {
                        visited.insert(dependent);
                        affected.push_back(dependent);
                        q.push(dependent);
                    }
                }
            }
        }
        
        return affected;
    }
    
    // ==================== 计算 ====================
    
    /**
     * @brief 设置变量值并更新依赖图
     */
    void setValue(uint32_t nodeId, double value) {
        auto* node = getNode(nodeId);
        if (!node) return;
        
        node->setValue(value);
        
        // 标记所有下游节点为脏
        auto affected = getAffectedNodes(nodeId);
        for (uint32_t id : affected) {
            auto* n = getNode(id);
            if (n) n->markDirty();
        }
    }
    
    /**
     * @brief 重新计算所有值
     */
    void recomputeAll() {
        auto order = getTopologicalOrder();
        
        for (uint32_t id : order) {
            auto* node = getNode(id);
            if (node && node->isDirty()) {
                computeNodeValue(node);
            }
        }
    }
    
    /**
     * @brief 增量更新（只更新受影响的节点）
     */
    void update(uint32_t changedNodeId) {
        // 从changedNodeId开始，拓扑顺序更新所有下游节点
        auto order = getTopologicalOrder();
        
        // 找到changedNodeId的位置
        auto it = std::find(order.begin(), order.end(), changedNodeId);
        if (it == order.end()) return;
        
        // 从changedNodeId开始更新
        bool startUpdating = false;
        for (; it != order.end(); ++it) {
            uint32_t id = *it;
            auto* node = getNode(id);
            if (id == changedNodeId) {
                startUpdating = true;
            }
            if (startUpdating && node && node->isDirty()) {
                computeNodeValue(node);
            }
        }
    }
    
    // ==================== 批量操作 ====================
    
    /**
     * @brief 批量设置值
     */
    void setValuesBatch(const std::vector<uint32_t>& nodeIds, const std::vector<double>& values) {
        for (size_t i = 0; i < nodeIds.size() && i < values.size(); ++i) {
            setValue(nodeIds[i], values[i]);
        }
    }
    
    /**
     * @brief 批量更新
     */
    void updateBatch(const std::vector<uint32_t>& changedNodeIds) {
        // 按拓扑排序顺序更新
        auto order = getTopologicalOrder();
        
        for (uint32_t changedId : changedNodeIds) {
            update(changedId);
        }
    }
    
    size_t size() const { return nodes_.size(); }

private:
    bool detectCycleDFS(uint32_t nodeId, std::unordered_set<uint32_t>& visiting, 
                        std::unordered_set<uint32_t>& visited) const {
        if (visiting.find(nodeId) != visiting.end()) {
            return true;  // 找到循环
        }
        
        if (visited.find(nodeId) != visited.end()) {
            return false;  // 已处理
        }
        
        visiting.insert(nodeId);
        
        auto* node = getNode(nodeId);
        if (node) {
            for (uint32_t dep : node->getDependencies()) {
                if (detectCycleDFS(dep, visiting, visited)) {
                    return true;
                }
            }
        }
        
        visiting.erase(nodeId);
        visited.insert(nodeId);
        return false;
    }
    
    void computeNodeValue(ParamNode* node) {
        if (!node) return;
        
        // 标记正在计算（用于循环检测）
        node->setComputing(true);
        
        // 计算值：基于依赖节点的值
        double value = 0.0;
        const auto& deps = node->getDependencies();
        
        if (deps.empty()) {
            // 无依赖，直接使用当前值
            value = node->getValue();
        } else {
            // 有依赖，计算表达式
            // 简化：求和
            for (uint32_t depId : deps) {
                auto* depNode = getNode(depId);
                if (depNode) {
                    value += depNode->getCachedValue();
                }
            }
        }
        
        node->setCachedValue(value);
        node->clearDirty();
        node->setComputing(false);
    }
};

// ============================================================================
// 参数表达式求值器
// ============================================================================

/**
 * @brief 参数表达式
 * 
 * 支持: +, -, *, /, (, ), 变量名, 数字
 */
class ParamExpression {
private:
    std::string expression_;
    std::vector<uint32_t> dependencyIds_;
    
public:
    ParamExpression(const std::string& expr) : expression_(expr) {}
    
    /**
     * @brief 解析表达式，提取依赖
     */
    void parse(ParamGraph* graph);
    
    /**
     * @brief 求值
     */
    double evaluate(ParamGraph* graph) const;
    
    const std::string& getExpression() const { return expression_; }
    const std::vector<uint32_t>& getDependencies() const { return dependencyIds_; }
};

} // namespace FastPCB

#endif // PCBPARAMGRAPH_H
