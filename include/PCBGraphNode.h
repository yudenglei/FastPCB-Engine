/**
 * @file PCBGraphNode.h
 * @brief 拓扑网络节点 - 用于网络关系管理
 */

#ifndef PCBGRAPHNODE_H
#define PCBGRAPHNODE_H

#include "PCBConfig.h"
#include <vector>
#include <unordered_map>
#include <memory>

namespace FastPCB {

// ============================================================================
// 拓扑节点类型
// ============================================================================

enum class GraphNodeType : uint8_t {
    COMPONENT = 0,    // 器件节点
    NET = 1,         // 网络节点
    PIN = 2,          // 引脚节点
    VIA = 3,          // 过孔节点
    TRACE = 4         // 走线节点
};

// ============================================================================
// 拓扑边
// ============================================================================

/**
 * @brief 拓扑边（连接关系）
 */
struct GraphEdge {
    uint32_t sourceId_;    // 源节点ID
    uint32_t targetId_;    // 目标节点ID
    uint32_t weight_;      // 权重（电阻、电容等）
    
    GraphEdge(uint32_t src, uint32_t dst, uint32_t w = 1) 
        : sourceId_(src), targetId_(dst), weight_(w) {}
};

// ============================================================================
// 拓扑节点
// ============================================================================

/**
 * @brief 拓扑节点
 */
class GraphNode {
private:
    uint32_t id_;
    GraphNodeType type_;
    std::vector<uint32_t> connectedNodes_;  // 连接的节点ID
    
public:
    GraphNode(uint32_t id, GraphNodeType type) : id_(id), type_(type) {}
    
    uint32_t getID() const { return id_; }
    GraphNodeType getType() const { return type_; }
    
    void addConnection(uint32_t nodeId) {
        connectedNodes_.push_back(nodeId);
    }
    
    void removeConnection(uint32_t nodeId) {
        connectedNodes_.erase(
            std::remove(connectedNodes_.begin(), connectedNodes_.end(), nodeId),
            connectedNodes_.end()
        );
    }
    
    const std::vector<uint32_t>& getConnections() const { 
        return connectedNodes_; 
    }
    
    size_t getDegree() const { return connectedNodes_.size(); }
};

// ============================================================================
// 拓扑图（网络关系）
// ============================================================================

/**
 * @brief 拓扑图
 * 
 * 用于管理PCB中的网络连接关系
 */
class PCBGraph {
private:
    std::unordered_map<uint32_t, std::unique_ptr<GraphNode>> nodes_;
    std::vector<GraphEdge> edges_;
    
public:
    /**
     * @brief 添加节点
     */
    void addNode(uint32_t id, GraphNodeType type) {
        nodes_[id] = std::make_unique<GraphNode>(id, type);
    }
    
    /**
     * @brief 添加边
     */
    void addEdge(uint32_t src, uint32_t dst, uint32_t weight = 1) {
        edges_.emplace_back(src, dst, weight);
        
        // 同时更新节点的连接关系
        auto it = nodes_.find(src);
        if (it != nodes_.end()) {
            it->second->addConnection(dst);
        }
    }
    
    /**
     * @brief 获取节点
     */
    GraphNode* getNode(uint32_t id) {
        auto it = nodes_.find(id);
        return it != nodes_.end() ? it->second.get() : nullptr;
    }
    
    /**
     * @brief 查找路径（BFS）
     */
    std::vector<uint32_t> findPath(uint32_t start, uint32_t end);
    
    /**
     * @brief 获取连通分量
     */
    std::vector<std::vector<uint32_t>> getConnectedComponents();
    
    /**
     * @brief 获取所有节点
     */
    const auto& getNodes() const { return nodes_; }
    
    /**
     * @brief 获取所有边
     */
    const auto& getEdges() const { return edges_; }
    
    size_t getNodeCount() const { return nodes_.size(); }
    size_t getEdgeCount() const { return edges_.size(); }
};

} // namespace FastPCB

#endif // PCBGRAPHNODE_H
