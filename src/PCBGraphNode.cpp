/**
 * @file PCBGraphNode.cpp
 * @brief 拓扑网络节点实现
 */

#include "PCBGraphNode.h"
#include <queue>
#include <set>

namespace FastPCB {

// ============================================================================
// PCBGraph Implementation
// ============================================================================

std::vector<uint32_t> PCBGraph::findPath(uint32_t start, uint32_t end) {
    if (nodes_.find(start) == nodes_.end() || 
        nodes_.find(end) == nodes_.end()) {
        return {};
    }
    
    // BFS
    std::queue<uint32_t> q;
    std::unordered_map<uint32_t, uint32_t> parent;
    
    q.push(start);
    parent[start] = start;
    
    while (!q.empty()) {
        uint32_t current = q.front();
        q.pop();
        
        if (current == end) {
            // 重建路径
            std::vector<uint32_t> path;
            uint32_t node = end;
            while (node != start) {
                path.push_back(node);
                node = parent[node];
            }
            path.push_back(start);
            std::reverse(path.begin(), path.end());
            return path;
        }
        
        auto* node = getNode(current);
        if (node) {
            for (uint32_t neighbor : node->getConnections()) {
                if (parent.find(neighbor) == parent.end()) {
                    parent[neighbor] = current;
                    q.push(neighbor);
                }
            }
        }
    }
    
    return {};  // 无路径
}

std::vector<std::vector<uint32_t>> PCBGraph::getConnectedComponents() {
    std::vector<std::vector<uint32_t>> components;
    std::set<uint32_t> visited;
    
    for (const auto& pair : nodes_) {
        uint32_t start = pair.first;
        
        if (visited.find(start) != visited.end()) {
            continue;
        }
        
        // BFS遍历
        std::vector<uint32_t> component;
        std::queue<uint32_t> q;
        
        q.push(start);
        visited.insert(start);
        
        while (!q.empty()) {
            uint32_t current = q.front();
            q.pop();
            component.push_back(current);
            
            auto* node = getNode(current);
            if (node) {
                for (uint32_t neighbor : node->getConnections()) {
                    if (visited.find(neighbor) == visited.end()) {
                        visited.insert(neighbor);
                        q.push(neighbor);
                    }
                }
            }
        }
        
        components.push_back(component);
    }
    
    return components;
}

} // namespace FastPCB
