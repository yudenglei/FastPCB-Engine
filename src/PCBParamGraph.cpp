/**
 * @file PCBParamGraph.cpp
 * @brief 参数化变量依赖图实现
 */

#include "PCBParamGraph.h"
#include <algorithm>
#include <cctype>

namespace FastPCB {

// ============================================================================
// ParamExpression Implementation
// ============================================================================

void ParamExpression::parse(ParamGraph* graph) {
    if (!graph) return;
    
    // 简单解析：提取变量名
    std::string varName;
    for (char c : expression_) {
        if (std::isalpha(c) || std::isdigit(c) || c == '_') {
            varName += c;
        } else if (!varName.empty()) {
            // 检查是否是有效变量名
            auto* node = graph->getNode(varName);
            if (node) {
                dependencyIds_.push_back(node->getID());
            }
            varName.clear();
        }
    }
    
    // 检查最后一个
    if (!varName.empty()) {
        auto* node = graph->getNode(varName);
        if (node) {
            dependencyIds_.push_back(node->getID());
        }
    }
}

double ParamExpression::evaluate(ParamGraph* graph) const {
    // 简化实现：返回依赖节点值的和
    if (!graph) return 0.0;
    
    double result = 0.0;
    for (uint32_t id : dependencyIds_) {
        auto* node = graph->getNode(id);
        if (node) {
            result += node->getCachedValue();
        }
    }
    
    return result;
}

} // namespace FastPCB
