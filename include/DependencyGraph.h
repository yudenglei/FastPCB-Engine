/**
 * @file DependencyGraph.h
 * @brief 有向依赖图 - 支持表达式变量和组件的增量更新
 */

#ifndef DEPENDENCYGRAPH_H
#define DEPENDENCYGRAPH_H

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <optional>
#include <string>
#include <functional>
#include <stdexcept>

namespace FastPCB {

// ============================================================================
// 节点类型
// ============================================================================

enum class GraphNodeType : uint8_t {
    Variable,   // 参数化变量 (可带表达式)
    Component   // 器件对象 (叶子节点，依赖变量)
};

// ============================================================================
// 节点状态
// ============================================================================

enum class NodeState : uint8_t {
    Clean,      // 已是最新
    Dirty,      // 需要重算
    Computing   // 正在计算中（用于循环检测）
};

// ============================================================================
// 依赖图节点
// ============================================================================

struct GraphNode {
    uint32_t            id;
    GraphNodeType       type;
    std::string         name;               // 名称
    std::string         expression;         // 表达式 (Variable类型)
    double              value = 0.0;         // 当前值
    double              cachedValue = 0.0;   // 缓存值
    NodeState           state = NodeState::Dirty;
    void*               userData = nullptr;  // 指向实际对象
    
    // 有向边
    std::unordered_set<uint32_t> dependencies;  // 依赖的节点 (上游)
    std::unordered_set<uint32_t> dependents;    // 依赖我的节点 (下游)
    
    GraphNode() = default;
    GraphNode(uint32_t id, GraphNodeType type, const std::string& name = "")
        : id(id), type(type), name(name) {}
};

// ============================================================================
// 依赖图异常
// ============================================================================

class CycleDetectedException : public std::runtime_error {
public:
    std::vector<uint32_t> cyclePath;
    
    CycleDetectedException(const std::string& msg, std::vector<uint32_t> path)
        : std::runtime_error(msg), cyclePath(std::move(path)) {}
};

// ============================================================================
// 依赖图
// ============================================================================

class DependencyGraph {
public:
    // ============ 节点管理 ============
    
    /**
     * @brief 添加节点
     * @return true if added, false if already exists
     */
    bool addNode(uint32_t id, GraphNodeType type, const std::string& name = "", void* userData = nullptr);
    
    /**
     * @brief 移除节点（同时移除所有相关边）
     */
    bool removeNode(uint32_t id);
    
    /**
     * @brief 获取节点
     */
    GraphNode* getNode(uint32_t id);
    const GraphNode* getNode(uint32_t id) const;
    
    /**
     * @brief 检查节点是否存在
     */
    bool hasNode(uint32_t id) const;
    
    /**
     * @brief 通过名称查找节点
     */
    GraphNode* getNodeByName(const std::string& name);
    const GraphNode* getNodeByName(const std::string& name) const;
    
    // ============ 表达式管理 (Variable专用) ============
    
    /**
     * @brief 设置变量表达式
     * @note 表达式格式: "var1 + var2 * 2 - 3" 支持 +, -, *, /, (, )
     */
    void setExpression(uint32_t varId, const std::string& expr);
    
    /**
     * @brief 获取变量表达式
     */
    const std::string& getExpression(uint32_t varId) const;
    
    // ============ 依赖关系管理 ============
    
    /**
     * @brief 添加依赖关系: dependent 依赖 dependency
     * @param dependent 依赖方（下游节点）
     * @param dependency 被依赖方（上游节点）
     * @param checkCycle 是否检测循环（默认检测）
     * @throws CycleDetectedException 如果会产生循环
     */
    void addDependency(uint32_t dependent, uint32_t dependency, bool checkCycle = true);
    
    /**
     * @brief 移除依赖关系
     */
    bool removeDependency(uint32_t dependent, uint32_t dependency);
    
    // ============ 脏标记与更新 ============
    
    /**
     * @brief 标记节点已变化，驱动所有下游节点
     */
    void markDirty(uint32_t nodeId);
    
    /**
     * @brief 标记节点为干净
     */
    void markClean(uint32_t nodeId);
    
    /**
     * @brief 获取脏节点列表
     */
    std::vector<uint32_t> getDirtyNodes() const;
    
    /**
     * @brief 获取拓扑排序后的更新顺序
     * @param changedNodes 可选的变更节点集合，未指定时返回所有脏节点
     */
    std::vector<uint32_t> getUpdateOrder(const std::vector<uint32_t>& changedNodes = {}) const;
    
    /**
     * @brief 获取直接依赖者
     */
    std::vector<uint32_t> getDirectDependents(uint32_t source) const;
    
    /**
     * @brief 获取传递依赖（所有下游节点）
     */
    std::vector<uint32_t> getAllDependents(uint32_t source) const;
    
    /**
     * @brief 获取直接依赖
     */
    std::vector<uint32_t> getDirectDependencies(uint32_t nodeId) const;
    
    // ============ 循环检测 ============
    
    /**
     * @brief 检测是否存在循环
     */
    bool hasCycle() const;
    
    /**
     * @brief 检查添加边是否会产生循环
     */
    bool wouldCreateCycle(uint32_t source, uint32_t target) const;
    
    /**
     * @brief 查找并返回循环路径
     * @return 循环路径，包含成环的节点ID序列
     */
    std::vector<uint32_t> findCycle() const;
    
    /**
     * @brief 验证图完整性（调试用）
     */
    bool validate() const;
    
    // ============ 求值 ============
    
    /**
     * @brief 计算节点值
     * @param nodeId 节点ID
     * @return 计算后的值
     */
    double evaluate(uint32_t nodeId);
    
    /**
     * @brief 重新计算所有脏节点
     */
    void refreshDirtyNodes();
    
    /**
     * @brief 重算所有节点
     */
    void recomputeAll();
    
    /**
     * @brief 设置求值回调（用于自定义计算逻辑）
     */
    void setEvaluator(uint32_t nodeId, std::function<double(const std::vector<double>&)> evaluator);
    
    // ============ 图查询 ============
    
    /**
     * @brief 获取完整的拓扑排序
     */
    std::optional<std::vector<uint32_t>> topologicalSort() const;
    
    /**
     * @brief 获取节点数量
     */
    size_t nodeCount() const { return nodes_.size(); }
    
    /**
     * @brief 获取边数量
     */
    size_t edgeCount() const { return edgeCount_; }
    
    /**
     * @brief 获取所有节点ID
     */
    std::vector<uint32_t> getAllNodes() const;
    
    /**
     * @brief 输出 Graphviz DOT 格式
     */
    void dumpDot(const std::string& filename) const;
    
    /**
     * @brief 清除所有节点和边
     */
    void clear();

private:
    std::unordered_map<uint32_t, GraphNode> nodes_;
    std::unordered_map<std::string, uint32_t> nameToId_;
    std::unordered_map<uint32_t, std::function<double(const std::vector<double>&)>> evaluators_;
    size_t edgeCount_ = 0;
    
    // 拓扑排序辅助
    std::vector<uint32_t> topologicalSortFrom(
        const std::unordered_set<uint32_t>& sources) const;
    
    // 循环检测辅助
    bool dfsDetectCycle(uint32_t nodeId, 
                        std::unordered_map<uint32_t, uint8_t>& visitState,
                        std::vector<uint32_t>& path) const;
    
    // 表达式解析
    std::vector<uint32_t> parseExpressionDependencies(const std::string& expr) const;
    
    // 表达式求值
    double evaluateExpression(const std::string& expr, 
                             const std::unordered_map<std::string, double>& values) const;
    
    // 简单表达式解析器
    struct ExprParser;
    double parseAndEval(const std::string& expr,
                        const std::unordered_map<std::string, double>& vars) const;
};

// ============================================================================
// 内联实现
// ============================================================================

inline bool DependencyGraph::hasNode(uint32_t id) const {
    return nodes_.count(id) > 0;
}

inline GraphNode* DependencyGraph::getNode(uint32_t id) {
    auto it = nodes_.find(id);
    return it != nodes_.end() ? &it->second : nullptr;
}

inline const GraphNode* DependencyGraph::getNode(uint32_t id) const {
    auto it = nodes_.find(id);
    return it != nodes_.end() ? &it->second : nullptr;
}

inline GraphNode* DependencyGraph::getNodeByName(const std::string& name) {
    auto it = nameToId_.find(name);
    return it != nameToId_.end() ? getNode(it->second) : nullptr;
}

inline const GraphNode* DependencyGraph::getNodeByName(const std::string& name) const {
    auto it = nameToId_.find(name);
    return it != nameToId_.end() ? getNode(it->second) : nullptr;
}

inline std::vector<uint32_t> DependencyGraph::getAllNodes() const {
    std::vector<uint32_t> result;
    result.reserve(nodes_.size());
    for (const auto& pair : nodes_) {
        result.push_back(pair.first);
    }
    return result;
}

} // namespace FastPCB

#endif // DEPENDENCYGRAPH_H
