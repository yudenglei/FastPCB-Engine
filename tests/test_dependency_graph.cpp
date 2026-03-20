/**
 * @file test_dependency_graph.cpp
 * @brief 依赖图单元测试
 */

#include "DependencyGraph.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <cmath>

using namespace FastPCB;

// ============================================================================
// 辅助函数
// ============================================================================

#define TEST(name) void test_##name()
#define RUN(name) do { \
    std::cout << "  " #name "... "; \
    test_##name(); \
    std::cout << "PASSED\n"; \
} while(0)

#define ASSERT(cond) do { \
    if (!(cond)) { \
        std::cerr << "\n    FAILED: " #cond << " at line " << __LINE__ << "\n"; \
        throw std::runtime_error("Assertion failed"); \
    } \
} while(0)

#define ASSERT_NEAR(a, b, tol) do { \
    if (std::abs((a) - (b)) > (tol)) { \
        std::cerr << "\n    FAILED: " << (a) << " != " << (b) << " (tol=" << (tol) << ")\n"; \
        throw std::runtime_error("Assertion failed"); \
    } \
} while(0)

// ============================================================================
// 测试：节点管理
// ============================================================================

TEST(node_management) {
    DependencyGraph graph;
    
    // 添加节点
    ASSERT(graph.addNode(1, GraphNodeType::Variable, "x"));
    ASSERT(graph.addNode(2, GraphNodeType::Variable, "y"));
    ASSERT(graph.addNode(3, GraphNodeType::Component, "comp1"));
    
    ASSERT(graph.nodeCount() == 3);
    ASSERT(graph.hasNode(1));
    ASSERT(graph.hasNode(2));
    ASSERT(graph.hasNode(3));
    
    // 重复添加会失败
    ASSERT(!graph.addNode(1, GraphNodeType::Variable));
    
    // 获取节点
    auto* node1 = graph.getNode(1);
    ASSERT(node1 != nullptr);
    ASSERT(node1->name == "x");
    ASSERT(node1->type == GraphNodeType::Variable);
    
    auto* node3 = graph.getNode(3);
    ASSERT(node3->type == GraphNodeType::Component);
    
    // 通过名称查找
    auto* nodeX = graph.getNodeByName("x");
    ASSERT(nodeX != nullptr);
    ASSERT(nodeX->id == 1);
    
    // 移除节点
    ASSERT(graph.removeNode(2));
    ASSERT(graph.nodeCount() == 2);
    ASSERT(!graph.hasNode(2));
}

// ============================================================================
// 测试：基础依赖关系
// ============================================================================

TEST(basic_dependencies) {
    DependencyGraph graph;
    
    // 创建: a -> x, a -> y (a 依赖 x 和 y)
    graph.addNode(1, GraphNodeType::Variable, "x");
    graph.addNode(2, GraphNodeType::Variable, "y");
    graph.addNode(3, GraphNodeType::Variable, "a");
    
    graph.addDependency(3, 1);  // a 依赖 x
    graph.addDependency(3, 2);  // a 依赖 y
    
    ASSERT(graph.edgeCount() == 2);
    
    // 检查依赖关系
    auto deps = graph.getDirectDependencies(3);
    ASSERT(deps.size() == 2);
    
    auto dependents = graph.getDirectDependents(1);
    ASSERT(dependents.size() == 1);
    ASSERT(dependents[0] == 3);
}

// ============================================================================
// 测试：拓扑排序
// ============================================================================

TEST(topological_sort) {
    DependencyGraph graph;
    
    // 创建链: x -> a -> b -> c
    graph.addNode(1, GraphNodeType::Variable, "x");
    graph.addNode(2, GraphNodeType::Variable, "a");
    graph.addNode(3, GraphNodeType::Variable, "b");
    graph.addNode(4, GraphNodeType::Variable, "c");
    
    graph.addDependency(2, 1);  // a -> x
    graph.addDependency(3, 2);  // b -> a
    graph.addDependency(4, 3);  // c -> b
    
    auto order = graph.topologicalSort();
    ASSERT(order.has_value());
    
    // 验证顺序: x 应该在 a 之前, a 在 b 之前, b 在 c 之前
    auto pos = [&order](uint32_t id) {
        return std::find(order->begin(), order->end(), id) - order->begin();
    };
    
    ASSERT(pos(1) < pos(2));
    ASSERT(pos(2) < pos(3));
    ASSERT(pos(3) < pos(4));
}

// ============================================================================
// 测试：获取更新顺序
// ============================================================================

TEST(update_order) {
    DependencyGraph graph;
    
    // 创建: x -> a -> b, x -> c
    graph.addNode(1, GraphNodeType::Variable, "x");
    graph.addNode(2, GraphNodeType::Variable, "a");
    graph.addNode(3, GraphNodeType::Variable, "b");
    graph.addNode(4, GraphNodeType::Variable, "c");
    
    graph.addDependency(2, 1);  // a 依赖 x
    graph.addDependency(3, 2);  // b 依赖 a
    graph.addDependency(4, 1);  // c 依赖 x
    
    auto order = graph.getUpdateOrder({1});  // 从 x 出发
    
    // x(1) 应该在最前面或不在列表中（因为它是源节点）
    // a(2) 和 c(4) 应该在 x 之后
    // b(3) 应该在 a 之后
    ASSERT(order.size() >= 3);
}

// ============================================================================
// 测试：循环检测
// ============================================================================

TEST(cycle_detection) {
    DependencyGraph graph;
    
    // 创建正常依赖
    graph.addNode(1, GraphNodeType::Variable, "x");
    graph.addNode(2, GraphNodeType::Variable, "y");
    graph.addNode(3, GraphNodeType::Variable, "z");
    
    graph.addDependency(2, 1);  // y 依赖 x
    graph.addDependency(3, 2);  // z 依赖 y
    
    ASSERT(!graph.hasCycle());
    
    // 添加循环: x 依赖 z (会成环)
    bool caught = false;
    try {
        graph.addDependency(1, 3);  // x 依赖 z -> x -> y -> z -> x
    } catch (const CycleDetectedException& e) {
        caught = true;
        ASSERT(e.cyclePath.size() > 0);
    }
    ASSERT(caught);
}

// ============================================================================
// 测试：自依赖检测
// ============================================================================

TEST(self_dependency) {
    DependencyGraph graph;
    
    graph.addNode(1, GraphNodeType::Variable, "x");
    
    bool caught = false;
    try {
        graph.addDependency(1, 1);  // 自依赖
    } catch (const CycleDetectedException& e) {
        caught = true;
    }
    ASSERT(caught);
}

// ============================================================================
// 测试：传递依赖
// ============================================================================

TEST(transitive_dependents) {
    DependencyGraph graph;
    
    // 创建: x -> a -> b -> c, a -> d
    graph.addNode(1, GraphNodeType::Variable, "x");
    graph.addNode(2, GraphNodeType::Variable, "a");
    graph.addNode(3, GraphNodeType::Variable, "b");
    graph.addNode(4, GraphNodeType::Variable, "c");
    graph.addNode(5, GraphNodeType::Variable, "d");
    
    graph.addDependency(2, 1);  // a -> x
    graph.addDependency(3, 2);  // b -> a
    graph.addDependency(4, 3);  // c -> b
    graph.addDependency(5, 2);  // d -> a
    
    auto allDeps = graph.getAllDependents(1);  // 从 x 出发
    
    // 应该找到 a, b, c, d
    ASSERT(allDeps.size() == 4);
    ASSERT(std::find(allDeps.begin(), allDeps.end(), 2) != allDeps.end());
    ASSERT(std::find(allDeps.begin(), allDeps.end(), 3) != allDeps.end());
    ASSERT(std::find(allDeps.begin(), allDeps.end(), 4) != allDeps.end());
    ASSERT(std::find(allDeps.begin(), allDeps.end(), 5) != allDeps.end());
}

// ============================================================================
// 测试：表达式求值
// ============================================================================

TEST(expression_evaluation) {
    DependencyGraph graph;
    
    // 创建变量
    graph.addNode(1, GraphNodeType::Variable, "a");
    graph.addNode(2, GraphNodeType::Variable, "b");
    graph.addNode(3, GraphNodeType::Variable, "c");
    graph.addNode(4, GraphNodeType::Variable, "result");
    
    // 设置值
    graph.getNode(1)->value = 10.0;
    graph.getNode(2)->value = 20.0;
    graph.getNode(3)->value = 5.0;
    
    // 设置表达式: result = a + b * c
    graph.setExpression(4, "a + b * c");
    
    // 设置依赖关系
    graph.addDependency(4, 1);  // result 依赖 a
    graph.addDependency(4, 2);  // result 依赖 b
    graph.addDependency(4, 3);  // result 依赖 c
    
    // 求值
    double result = graph.evaluate(4);
    
    // a + b * c = 10 + 20 * 5 = 110
    ASSERT_NEAR(result, 110.0, 0.001);
}

// ============================================================================
// 测试：表达式依赖解析
// ============================================================================

TEST(expression_dependency_parsing) {
    DependencyGraph graph;
    
    // 创建变量
    graph.addNode(1, GraphNodeType::Variable, "x");
    graph.addNode(2, GraphNodeType::Variable, "y");
    graph.addNode(3, GraphNodeType::Variable, "z");
    graph.addNode(4, GraphNodeType::Variable, "result");
    
    // x = 10, y = 20
    graph.getNode(1)->value = 10.0;
    graph.getNode(2)->value = 20.0;
    
    // 设置表达式，自动解析依赖
    graph.setExpression(4, "x + y");
    
    // 应该自动添加依赖关系
    auto deps = graph.getDirectDependencies(4);
    ASSERT(deps.size() == 2);
    ASSERT(std::find(deps.begin(), deps.end(), 1) != deps.end());
    ASSERT(std::find(deps.begin(), deps.end(), 2) != deps.end());
    
    // 求值: x + y = 30
    double result = graph.evaluate(4);
    ASSERT_NEAR(result, 30.0, 0.001);
}

// ============================================================================
// 测试：复杂表达式
// ============================================================================

TEST(complex_expressions) {
    DependencyGraph graph;
    
    graph.addNode(1, GraphNodeType::Variable, "a");
    graph.addNode(2, GraphNodeType::Variable, "b");
    graph.addNode(3, GraphNodeType::Variable, "c");
    graph.addNode(4, GraphNodeType::Variable, "result");
    
    graph.getNode(1)->value = 10.0;
    graph.getNode(2)->value = 3.0;
    graph.getNode(3)->value = 2.0;
    
    // (a + b) * c = (10 + 3) * 2 = 26
    graph.setExpression(4, "(a + b) * c");
    graph.addDependency(4, 1);
    graph.addDependency(4, 2);
    graph.addDependency(4, 3);
    
    double result = graph.evaluate(4);
    ASSERT_NEAR(result, 26.0, 0.001);
    
    // a - b / c = 10 - 3 / 2 = 8.5
    graph.getNode(4)->expression = "a - b / c";
    graph.getNode(4)->state = NodeState::Dirty;
    result = graph.evaluate(4);
    ASSERT_NEAR(result, 8.5, 0.001);
}

// ============================================================================
// 测试：脏标记和增量更新
// ============================================================================

TEST(dirty_marking) {
    DependencyGraph graph;
    
    // 创建: x -> a -> b
    graph.addNode(1, GraphNodeType::Variable, "x");
    graph.addNode(2, GraphNodeType::Variable, "a");
    graph.addNode(3, GraphNodeType::Variable, "b");
    
    graph.addDependency(2, 1);
    graph.addDependency(3, 2);
    
    // 初始状态，所有节点为脏
    auto dirty = graph.getDirtyNodes();
    ASSERT(dirty.size() == 3);
    
    // 求值后变为干净
    graph.recomputeAll();
    
    dirty = graph.getDirtyNodes();
    ASSERT(dirty.empty());
    
    // 修改 x，标记为脏
    graph.getNode(1)->value = 100.0;
    graph.markDirty(1);
    
    dirty = graph.getDirtyNodes();
    ASSERT(dirty.size() == 3);  // x, a, b 都应该变脏
    
    // 增量更新
    graph.refreshDirtyNodes();
    
    dirty = graph.getDirtyNodes();
    ASSERT(dirty.empty());
}

// ============================================================================
// 测试：自定义求值器
// ============================================================================

TEST(custom_evaluator) {
    DependencyGraph graph;
    
    graph.addNode(1, GraphNodeType::Variable, "x");
    graph.addNode(2, GraphNodeType::Variable, "y");
    graph.addNode(3, GraphNodeType::Variable, "max");
    
    graph.getNode(1)->value = 10.0;
    graph.getNode(2)->value = 20.0;
    
    // 设置自定义求值器：取最大值
    graph.setEvaluator(3, [](const std::vector<double>& deps) {
        return std::max(deps[0], deps[1]);
    });
    
    graph.addDependency(3, 1);
    graph.addDependency(3, 2);
    
    double result = graph.evaluate(3);
    ASSERT_NEAR(result, 20.0, 0.001);
}

// ============================================================================
// 测试：组件节点
// ============================================================================

TEST(component_nodes) {
    DependencyGraph graph;
    
    // 变量
    graph.addNode(1, GraphNodeType::Variable, "width");
    graph.addNode(2, GraphNodeType::Variable, "height");
    
    // 组件（依赖变量）
    graph.addNode(3, GraphNodeType::Component, "resistor1");
    
    graph.addDependency(3, 1);  // 组件依赖 width
    graph.addDependency(3, 2);  // 组件依赖 height
    
    // width 变化时，组件应该被标记为脏
    graph.recomputeAll();
    
    graph.getNode(1)->value = 100.0;
    graph.markDirty(1);
    
    auto dirty = graph.getDirtyNodes();
    ASSERT(dirty.size() == 2);  // width 和 resistor1
}

// ============================================================================
// 测试：批量更新
// ============================================================================

TEST(batch_update) {
    DependencyGraph graph;
    
    // 创建: x -> a, y -> b, a -> c, b -> c
    graph.addNode(1, GraphNodeType::Variable, "x");
    graph.addNode(2, GraphNodeType::Variable, "y");
    graph.addNode(3, GraphNodeType::Variable, "a");
    graph.addNode(4, GraphNodeType::Variable, "b");
    graph.addNode(5, GraphNodeType::Variable, "c");
    
    graph.addDependency(3, 1);  // a 依赖 x
    graph.addDependency(4, 2);  // b 依赖 y
    graph.addDependency(5, 3);  // c 依赖 a
    graph.addDependency(5, 4);  // c 依赖 b
    
    graph.recomputeAll();
    
    // 同时修改 x 和 y
    graph.getNode(1)->value = 100.0;
    graph.getNode(2)->value = 200.0;
    
    // 批量更新
    graph.refreshDirtyNodes();
    
    ASSERT(graph.getNode(3)->state == NodeState::Clean);
    ASSERT(graph.getNode(4)->state == NodeState::Clean);
    ASSERT(graph.getNode(5)->state == NodeState::Clean);
}

// ============================================================================
// 测试：DOT输出
// ============================================================================

TEST(dot_output) {
    DependencyGraph graph;
    
    graph.addNode(1, GraphNodeType::Variable, "x");
    graph.addNode(2, GraphNodeType::Variable, "y");
    graph.addNode(3, GraphNodeType::Variable, "z");
    
    graph.addDependency(2, 1);
    graph.addDependency(3, 2);
    
    // 应该能够生成 DOT 文件而不崩溃
    graph.dumpDot("test_output.dot");
    
    // 验证文件存在
    FILE* f = fopen("test_output.dot", "r");
    ASSERT(f != nullptr);
    if (f) fclose(f);
    
    // 清理
    remove("test_output.dot");
}

// ============================================================================
// 测试：图验证
// ============================================================================

TEST(validation) {
    DependencyGraph graph;
    
    graph.addNode(1, GraphNodeType::Variable, "x");
    graph.addNode(2, GraphNodeType::Variable, "y");
    graph.addNode(3, GraphNodeType::Variable, "z");
    
    graph.addDependency(2, 1);
    graph.addDependency(3, 2);
    
    ASSERT(graph.validate());
    
    // 移除节点后验证
    graph.removeNode(1);
    ASSERT(graph.validate());
}

// ============================================================================
// 测试：移除依赖关系
// ============================================================================

TEST(remove_dependency) {
    DependencyGraph graph;
    
    graph.addNode(1, GraphNodeType::Variable, "x");
    graph.addNode(2, GraphNodeType::Variable, "y");
    graph.addNode(3, GraphNodeType::Variable, "z");
    
    graph.addDependency(3, 1);
    graph.addDependency(3, 2);
    
    ASSERT(graph.edgeCount() == 2);
    
    graph.removeDependency(3, 1);
    
    ASSERT(graph.edgeCount() == 1);
    
    auto deps = graph.getDirectDependencies(3);
    ASSERT(deps.size() == 1);
    ASSERT(deps[0] == 2);
}

// ============================================================================
// 测试：清除图
// ============================================================================

TEST(clear_graph) {
    DependencyGraph graph;
    
    graph.addNode(1, GraphNodeType::Variable, "x");
    graph.addNode(2, GraphNodeType::Variable, "y");
    graph.addDependency(2, 1);
    
    ASSERT(graph.nodeCount() == 2);
    ASSERT(graph.edgeCount() == 1);
    
    graph.clear();
    
    ASSERT(graph.nodeCount() == 0);
    ASSERT(graph.edgeCount() == 0);
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "========================================\n";
    std::cout << "  DependencyGraph Unit Tests\n";
    std::cout << "========================================\n\n";
    
    int passed = 0;
    int failed = 0;
    
    auto runTest = [&](auto testFn, const char* name) {
        try {
            std::cout << "  " << name << "... ";
            testFn();
            std::cout << "PASSED\n";
            ++passed;
        } catch (const std::exception& e) {
            std::cout << "FAILED: " << e.what() << "\n";
            ++failed;
        }
    };
    
    // 运行所有测试
    runTest(test_node_management, "node_management");
    runTest(test_basic_dependencies, "basic_dependencies");
    runTest(test_topological_sort, "topological_sort");
    runTest(test_update_order, "update_order");
    runTest(test_cycle_detection, "cycle_detection");
    runTest(test_self_dependency, "self_dependency");
    runTest(test_transitive_dependents, "transitive_dependents");
    runTest(test_expression_evaluation, "expression_evaluation");
    runTest(test_expression_dependency_parsing, "expression_dependency_parsing");
    runTest(test_complex_expressions, "complex_expressions");
    runTest(test_dirty_marking, "dirty_marking");
    runTest(test_custom_evaluator, "custom_evaluator");
    runTest(test_component_nodes, "component_nodes");
    runTest(test_batch_update, "batch_update");
    runTest(test_dot_output, "dot_output");
    runTest(test_validation, "validation");
    runTest(test_remove_dependency, "remove_dependency");
    runTest(test_clear_graph, "clear_graph");
    
    std::cout << "\n========================================\n";
    std::cout << "  Results: " << passed << " passed, " << failed << " failed\n";
    std::cout << "========================================\n";
    
    return failed > 0 ? 1 : 0;
}
