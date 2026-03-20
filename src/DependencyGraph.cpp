/**
 * @file DependencyGraph.cpp
 * @brief 有向依赖图实现
 */

#include "DependencyGraph.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <stack>
#include <cmath>

namespace FastPCB {

// ============================================================================
// 节点管理
// ============================================================================

bool DependencyGraph::addNode(uint32_t id, GraphNodeType type, 
                               const std::string& name, void* userData) {
    if (nodes_.count(id)) return false;
    
    GraphNode node(id, type, name);
    node.userData = userData;
    nodes_[id] = node;
    
    if (!name.empty()) {
        nameToId_[name] = id;
    }
    
    return true;
}

bool DependencyGraph::removeNode(uint32_t id) {
    auto it = nodes_.find(id);
    if (it == nodes_.end()) return false;
    
    const auto& node = it->second;
    
    // 移除所有相关边
    for (uint32_t dep : node.dependencies) {
        nodes_[dep].dependents.erase(id);
        --edgeCount_;
    }
    for (uint32_t dep : node.dependents) {
        nodes_[dep].dependencies.erase(id);
        --edgeCount_;
    }
    
    // 从名称索引移除
    if (!node.name.empty()) {
        nameToId_.erase(node.name);
    }
    
    nodes_.erase(it);
    evaluators_.erase(id);
    
    return true;
}

void DependencyGraph::clear() {
    nodes_.clear();
    nameToId_.clear();
    evaluators_.clear();
    edgeCount_ = 0;
}

// ============================================================================
// 表达式管理
// ============================================================================

void DependencyGraph::setExpression(uint32_t varId, const std::string& expr) {
    auto* node = getNode(varId);
    if (!node || node->type != GraphNodeType::Variable) return;
    
    node->expression = expr;
    node->state = NodeState::Dirty;
    
    // 解析依赖并更新图
    auto deps = parseExpressionDependencies(expr);
    
    // 移除旧的依赖关系
    for (uint32_t oldDep : node->dependencies) {
        nodes_[oldDep].dependents.erase(varId);
        --edgeCount_;
    }
    node->dependencies.clear();
    
    // 添加新的依赖关系
    for (uint32_t depId : deps) {
        if (nodes_.count(depId) && depId != varId) {
            // 检测循环
            if (!wouldCreateCycle(varId, depId)) {
                node->dependencies.insert(depId);
                nodes_[depId].dependents.insert(varId);
                ++edgeCount_;
            }
        }
    }
}

const std::string& DependencyGraph::getExpression(uint32_t varId) const {
    static std::string empty;
    auto it = nodes_.find(varId);
    return (it != nodes_.end()) ? it->second.expression : empty;
}

// ============================================================================
// 依赖关系管理
// ============================================================================

void DependencyGraph::addDependency(uint32_t dependent, uint32_t dependency, bool checkCycle) {
    if (!nodes_.count(dependent) || !nodes_.count(dependency)) return;
    if (dependent == dependency) {
        throw CycleDetectedException("Self-dependency detected", {dependent, dependency});
    }
    
    // 检查是否会产生循环
    if (checkCycle && wouldCreateCycle(dependency, dependent)) {
        auto cyclePath = findCycle();
        throw CycleDetectedException("Cycle detected in dependency graph", cyclePath);
    }
    
    // 检查是否已存在
    if (nodes_[dependent].dependencies.count(dependency)) return;
    
    // 添加边
    nodes_[dependent].dependencies.insert(dependency);
    nodes_[dependency].dependents.insert(dependent);
    ++edgeCount_;
    
    // 标记下游节点为脏
    markDirty(dependent);
}

bool DependencyGraph::removeDependency(uint32_t dependent, uint32_t dependency) {
    if (!nodes_.count(dependent) || !nodes_.count(dependency)) return false;
    
    if (nodes_[dependent].dependencies.erase(dependency)) {
        nodes_[dependency].dependents.erase(dependent);
        --edgeCount_;
        return true;
    }
    return false;
}

// ============================================================================
// 脏标记与更新
// ============================================================================

void DependencyGraph::markDirty(uint32_t nodeId) {
    auto it = nodes_.find(nodeId);
    if (it != nodes_.end()) {
        it->second.state = NodeState::Dirty;
        
        // 递归标记所有下游节点
        std::queue<uint32_t> queue;
        queue.push(nodeId);
        
        std::unordered_set<uint32_t> visited;
        visited.insert(nodeId);
        
        while (!queue.empty()) {
            uint32_t current = queue.front();
            queue.pop();
            
            for (uint32_t dep : nodes_[current].dependents) {
                if (!visited.count(dep)) {
                    visited.insert(dep);
                    nodes_[dep].state = NodeState::Dirty;
                    queue.push(dep);
                }
            }
        }
    }
}

void DependencyGraph::markClean(uint32_t nodeId) {
    auto it = nodes_.find(nodeId);
    if (it != nodes_.end()) {
        it->second.state = NodeState::Clean;
    }
}

std::vector<uint32_t> DependencyGraph::getDirtyNodes() const {
    std::vector<uint32_t> dirty;
    for (const auto& pair : nodes_) {
        if (pair.second.state == NodeState::Dirty) {
            dirty.push_back(pair.first);
        }
    }
    return dirty;
}

std::vector<uint32_t> DependencyGraph::getUpdateOrder(
    const std::vector<uint32_t>& changedNodes) const {
    
    std::unordered_set<uint32_t> sources(changedNodes.begin(), changedNodes.end());
    
    // 如果没有指定变更节点，使用所有脏节点
    if (sources.empty()) {
        for (const auto& pair : nodes_) {
            if (pair.second.state == NodeState::Dirty) {
                sources.insert(pair.first);
            }
        }
    }
    
    return topologicalSortFrom(sources);
}

std::vector<uint32_t> DependencyGraph::topologicalSortFrom(
    const std::unordered_set<uint32_t>& sources) const {
    
    std::vector<uint32_t> result;
    result.reserve(nodes_.size());
    
    // 计算入度
    std::unordered_map<uint32_t, size_t> inDegree;
    for (const auto& pair : nodes_) {
        inDegree[pair.first] = pair.second.dependencies.size();
    }
    
    // BFS 拓扑排序
    std::queue<uint32_t> queue;
    
    // 从源节点开始，入度为0的节点入队
    for (const auto& pair : inDegree) {
        if (pair.second == 0) {
            queue.push(pair.first);
        }
    }
    
    while (!queue.empty()) {
        uint32_t current = queue.front();
        queue.pop();
        result.push_back(current);
        
        for (uint32_t dep : nodes_.at(current).dependents) {
            if (inDegree[dep]-- == 1) {
                queue.push(dep);
            }
        }
    }
    
    return result;
}

std::vector<uint32_t> DependencyGraph::getDirectDependents(uint32_t source) const {
    auto it = nodes_.find(source);
    if (it == nodes_.end()) return {};
    return std::vector<uint32_t>(it->second.dependents.begin(),
                                  it->second.dependents.end());
}

std::vector<uint32_t> DependencyGraph::getAllDependents(uint32_t source) const {
    std::vector<uint32_t> result;
    std::unordered_set<uint32_t> visited;
    std::queue<uint32_t> queue;
    
    queue.push(source);
    visited.insert(source);
    
    while (!queue.empty()) {
        uint32_t current = queue.front();
        queue.pop();
        
        for (uint32_t dep : nodes_.at(current).dependents) {
            if (!visited.count(dep)) {
                visited.insert(dep);
                result.push_back(dep);
                queue.push(dep);
            }
        }
    }
    
    return result;
}

std::vector<uint32_t> DependencyGraph::getDirectDependencies(uint32_t nodeId) const {
    auto it = nodes_.find(nodeId);
    if (it == nodes_.end()) return {};
    return std::vector<uint32_t>(it->second.dependencies.begin(),
                                  it->second.dependencies.end());
}

// ============================================================================
// 循环检测
// ============================================================================

bool DependencyGraph::hasCycle() const {
    std::unordered_map<uint32_t, uint8_t> visitState;
    std::vector<uint32_t> path;
    
    for (const auto& pair : nodes_) {
        if (!visitState.count(pair.first)) {
            if (dfsDetectCycle(pair.first, visitState, path)) {
                return true;
            }
        }
    }
    return false;
}

bool DependencyGraph::dfsDetectCycle(uint32_t nodeId,
                                     std::unordered_map<uint32_t, uint8_t>& visitState,
                                     std::vector<uint32_t>& path) const {
    visitState[nodeId] = 1; // 标记为访问中
    path.push_back(nodeId);
    
    auto it = nodes_.find(nodeId);
    if (it != nodes_.end()) {
        for (uint32_t next : it->second.dependents) {
            auto nextState = visitState.find(next);
            if (nextState == visitState.end()) {
                if (dfsDetectCycle(next, visitState, path)) {
                    return true;
                }
            } else if (nextState->second == 1) {
                // 发现回边
                path.push_back(next);
                return true;
            }
        }
    }
    
    path.pop_back();
    visitState[nodeId] = 2; // 标记为已完成
    return false;
}

bool DependencyGraph::wouldCreateCycle(uint32_t dependent, uint32_t dependency) const {
    // 添加依赖: dependent -> dependency
    // 如果从 dependency 可以走到 dependent，就说明会成环
    // 因为 dependency -> ... -> dependent -> dependency 形成环
    
    std::unordered_set<uint32_t> visited;
    std::queue<uint32_t> queue;
    queue.push(dependency);
    
    while (!queue.empty()) {
        uint32_t current = queue.front();
        queue.pop();
        
        if (current == dependent) return true;
        
        if (visited.count(current)) continue;
        visited.insert(current);
        
        auto it = nodes_.find(current);
        if (it != nodes_.end()) {
            for (uint32_t next : it->second.dependents) {
                if (!visited.count(next)) {
                    queue.push(next);
                }
            }
        }
    }
    
    return false;
}

std::vector<uint32_t> DependencyGraph::findCycle() const {
    std::unordered_map<uint32_t, uint8_t> visitState;
    std::vector<uint32_t> path;
    
    for (const auto& pair : nodes_) {
        if (!visitState.count(pair.first)) {
            path.clear();
            if (dfsDetectCycle(pair.first, visitState, path)) {
                // 找到环，返回从环起点到终点的路径
                // 去掉最后的重复节点
                if (!path.empty() && path.back() == path.front()) {
                    path.pop_back();
                }
                return path;
            }
        }
    }
    return {};
}

bool DependencyGraph::validate() const {
    // 验证边的双向一致性
    for (const auto& pair : nodes_) {
        for (uint32_t dep : pair.second.dependencies) {
            if (!nodes_.count(dep)) {
                return false; // 依赖的目标不存在
            }
            if (!nodes_.at(dep).dependents.count(pair.first)) {
                return false; // 边不一致
            }
        }
        for (uint32_t dep : pair.second.dependents) {
            if (!nodes_.count(dep)) {
                return false;
            }
            if (!nodes_.at(dep).dependencies.count(pair.first)) {
                return false;
            }
        }
    }
    return true;
}

// ============================================================================
// 求值
// ============================================================================

double DependencyGraph::evaluate(uint32_t nodeId) {
    auto* node = getNode(nodeId);
    if (!node) return 0.0;
    
    // 如果是干净状态，直接返回缓存值
    if (node->state == NodeState::Clean) {
        return node->cachedValue;
    }
    
    // 如果正在计算，说明有循环
    if (node->state == NodeState::Computing) {
        throw CycleDetectedException("Circular dependency detected during evaluation",
                                     findCycle());
    }
    
    // 标记为计算中
    node->state = NodeState::Computing;
    
    double result = 0.0;
    
    // 如果有自定义求值器，优先使用
    auto evalIt = evaluators_.find(nodeId);
    if (evalIt != evaluators_.end()) {
        std::vector<double> depValues;
        for (uint32_t depId : node->dependencies) {
            depValues.push_back(evaluate(depId));
        }
        result = evalIt->second(depValues);
    }
    // 否则如果有表达式，使用表达式求值
    else if (!node->expression.empty()) {
        std::unordered_map<std::string, double> varValues;
        for (uint32_t depId : node->dependencies) {
            auto* depNode = getNode(depId);
            if (depNode) {
                varValues[depNode->name] = evaluate(depId);
            }
        }
        result = parseAndEval(node->expression, varValues);
    }
    // 否则使用当前值
    else {
        result = node->value;
    }
    
    node->cachedValue = result;
    node->state = NodeState::Clean;
    
    return result;
}

void DependencyGraph::refreshDirtyNodes() {
    auto dirtyNodes = getDirtyNodes();
    auto order = getUpdateOrder(dirtyNodes);
    
    for (uint32_t id : order) {
        evaluate(id);
    }
}

void DependencyGraph::recomputeAll() {
    // 先将所有节点标记为脏
    for (auto& pair : nodes_) {
        pair.second.state = NodeState::Dirty;
    }
    
    // 然后按拓扑顺序求值
    auto order = topologicalSortFrom({});
    for (uint32_t id : order) {
        evaluate(id);
    }
}

void DependencyGraph::setEvaluator(uint32_t nodeId,
                                    std::function<double(const std::vector<double>&)> evaluator) {
    if (hasNode(nodeId)) {
        evaluators_[nodeId] = evaluator;
        markDirty(nodeId);
    }
}

// ============================================================================
// 图查询
// ============================================================================

std::optional<std::vector<uint32_t>> DependencyGraph::topologicalSort() const {
    if (hasCycle()) {
        return std::nullopt;
    }
    
    return topologicalSortFrom({});
}

void DependencyGraph::dumpDot(const std::string& filename) const {
    std::ofstream file(filename);
    file << "digraph DependencyGraph {\n";
    file << "  rankdir=TB;\n";
    file << "  node [shape=box];\n\n";
    
    // 节点定义
    for (const auto& pair : nodes_) {
        std::string shape = (pair.second.type == GraphNodeType::Variable) ? "ellipse" : "box";
        std::string color;
        switch (pair.second.state) {
            case NodeState::Dirty:    color = "red"; break;
            case NodeState::Computing: color = "orange"; break;
            case NodeState::Clean:    color = "black"; break;
        }
        
        file << "  n" << pair.first << " [label=\"";
        if (!pair.second.name.empty()) {
            file << pair.second.name << " ";
        }
        file << "(" << pair.first << ")\\nval=" << pair.second.cachedValue;
        if (!pair.second.expression.empty()) {
            file << "\\nexpr=" << pair.second.expression;
        }
        file << "\" shape=" << shape << " color=" << color << "];\n";
    }
    
    file << "\n";
    
    // 边定义
    for (const auto& pair : nodes_) {
        for (uint32_t dep : pair.second.dependencies) {
            file << "  n" << dep << " -> n" << pair.first << ";\n";
        }
    }
    
    file << "}\n";
    file.close();
}

// ============================================================================
// 表达式解析
// ============================================================================

std::vector<uint32_t> DependencyGraph::parseExpressionDependencies(
    const std::string& expr) const {
    
    std::vector<uint32_t> deps;
    std::string varName;
    
    for (size_t i = 0; i < expr.size(); ++i) {
        char c = expr[i];
        
        if (std::isalpha(c) || c == '_' || (std::isdigit(c) && !varName.empty())) {
            varName += c;
        } else {
            if (!varName.empty()) {
                // 查找变量
                auto it = nameToId_.find(varName);
                if (it != nameToId_.end()) {
                    deps.push_back(it->second);
                }
                varName.clear();
            }
        }
    }
    
    // 检查最后一个
    if (!varName.empty()) {
        auto it = nameToId_.find(varName);
        if (it != nameToId_.end()) {
            deps.push_back(it->second);
        }
    }
    
    return deps;
}

// ============================================================================
// 表达式求值器
// ============================================================================

struct DependencyGraph::ExprParser {
    const std::unordered_map<std::string, double>& vars;
    std::string expr;
    size_t pos = 0;
    
    ExprParser(const std::string& e, const std::unordered_map<std::string, double>& v)
        : expr(e), vars(v) {}
    
    char peek() {
        while (pos < expr.size() && std::isspace(expr[pos])) ++pos;
        return pos < expr.size() ? expr[pos] : '\0';
    }
    
    char get() {
        while (pos < expr.size() && std::isspace(expr[pos])) ++pos;
        return pos < expr.size() ? expr[pos++] : '\0';
    }
    
    void skipWhitespace() {
        while (pos < expr.size() && std::isspace(expr[pos])) ++pos;
    }
    
    double parseExpression() {
        double result = parseAddSub();
        return result;
    }
    
    double parseAddSub() {
        double left = parseMulDiv();
        
        while (true) {
            skipWhitespace();
            char op = peek();
            if (op == '+' || op == '-') {
                get();
                double right = parseMulDiv();
                if (op == '+') left += right;
                else left -= right;
            } else {
                break;
            }
        }
        
        return left;
    }
    
    double parseMulDiv() {
        double left = parseUnary();
        
        while (true) {
            skipWhitespace();
            char op = peek();
            if (op == '*' || op == '/') {
                get();
                double right = parseUnary();
                if (op == '*') left *= right;
                else {
                    if (std::abs(right) < 1e-10) left = 0; // 除以零处理
                    else left /= right;
                }
            } else {
                break;
            }
        }
        
        return left;
    }
    
    double parseUnary() {
        skipWhitespace();
        char op = peek();
        if (op == '-') {
            get();
            return -parsePrimary();
        }
        return parsePrimary();
    }
    
    double parsePrimary() {
        skipWhitespace();
        char c = peek();
        
        // 括号
        if (c == '(') {
            get(); // 跳过 '('
            double result = parseExpression();
            skipWhitespace();
            if (peek() == ')') get(); // 跳过 ')'
            return result;
        }
        
        // 数字
        if (std::isdigit(c) || c == '.') {
            std::string num;
            while (std::isdigit(c) || c == '.' || c == 'e' || c == 'E' || 
                   c == '+' || c == '-') {
                if ((c == '+' || c == '-') && !num.empty() && 
                    num.back() != 'e' && num.back() != 'E') {
                    break;
                }
                num += c;
                c = get();
            }
            return std::stod(num);
        }
        
        // 变量名
        if (std::isalpha(c) || c == '_') {
            std::string name;
            while (std::isalnum(c) || c == '_') {
                name += c;
                c = get();
            }
            
            auto it = vars.find(name);
            if (it != vars.end()) {
                return it->second;
            }
            // 未定义的变量返回0
            return 0.0;
        }
        
        return 0.0;
    }
};

double DependencyGraph::parseAndEval(
    const std::string& expr,
    const std::unordered_map<std::string, double>& vars) const {
    
    ExprParser parser(expr, vars);
    return parser.parseExpression();
}

} // namespace FastPCB
