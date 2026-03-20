/**
 * @file test_dependency_graph_standalone.cpp
 * @brief 依赖图独立测试（无外部依赖）
 * 
 * 编译: g++ -std=c++17 -I../include -o test_dep test_dependency_graph_standalone.cpp ../src/DependencyGraph.cpp
 * 运行: ./test_dep
 */

#include "DependencyGraph.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <cmath>
#include <algorithm>

using namespace FastPCB;

// ============================================================================
// 辅助宏
// ============================================================================

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
// 测试用例
// ============================================================================

void test_node_management() {
    std::cout << "  test_node_management... ";
    DependencyGraph graph;
    
    ASSERT(graph.addNode(1, GraphNodeType::Variable, "x"));
    ASSERT(graph.addNode(2, GraphNodeType::Variable, "y"));
    ASSERT(graph.addNode(3, GraphNodeType::Component, "comp1"));
    
    ASSERT(graph.nodeCount() == 3);
    ASSERT(graph.hasNode(1));
    ASSERT(!graph.addNode(1, GraphNodeType::Variable)); // 重复添加失败
    
    auto* node1 = graph.getNode(1);
    ASSERT(node1 != nullptr);
    ASSERT(node1->name == "x");
    ASSERT(node1->type == GraphNodeType::Variable);
    
    auto* nodeX = graph.getNodeByName("x");
    ASSERT(nodeX != nullptr);
    ASSERT(nodeX->id == 1);
    
    graph.removeNode(2);
    ASSERT(graph.nodeCount() == 2);
    ASSERT(!graph.hasNode(2));
    
    std::cout << "PASSED\n";
}

void test_basic_dependencies() {
    std::cout << "  test_basic_dependencies... ";
    DependencyGraph graph;
    
    graph.addNode(1, GraphNodeType::Variable, "x");
    graph.addNode(2, GraphNodeType::Variable, "y");
    graph.addNode(3, GraphNodeType::Variable, "a");
    
    graph.addDependency(3, 1);
    graph.addDependency(3, 2);
    
    ASSERT(graph.edgeCount() == 2);
    
    auto deps = graph.getDirectDependencies(3);
    ASSERT(deps.size() == 2);
    
    auto dependents = graph.getDirectDependents(1);
    ASSERT(dependents.size() == 1);
    ASSERT(dependents[0] == 3);
    
    std::cout << "PASSED\n";
}

void test_topological_sort() {
    std::cout << "  test_topological_sort... ";
    DependencyGraph graph;
    
    // x -> a -> b -> c
    graph.addNode(1, GraphNodeType::Variable, "x");
    graph.addNode(2, GraphNodeType::Variable, "a");
    graph.addNode(3, GraphNodeType::Variable, "b");
    graph.addNode(4, GraphNodeType::Variable, "c");
    
    graph.addDependency(2, 1);
    graph.addDependency(3, 2);
    graph.addDependency(4, 3);
    
    auto order = graph.topologicalSort();
    ASSERT(order.has_value());
    
    auto pos = [&](uint32_t id) {
        return std::find(order->begin(), order->end(), id) - order->begin();
    };
    
    ASSERT(pos(1) < pos(2));
    ASSERT(pos(2) < pos(3));
    ASSERT(pos(3) < pos(4));
    
    std::cout << "PASSED\n";
}

void test_cycle_detection() {
    std::cout << "  test_cycle_detection... ";
    DependencyGraph graph;
    
    graph.addNode(1, GraphNodeType::Variable, "x");
    graph.addNode(2, GraphNodeType::Variable, "y");
    graph.addNode(3, GraphNodeType::Variable, "z");
    
    graph.addDependency(2, 1);
    graph.addDependency(3, 2);
    
    ASSERT(!graph.hasCycle());
    
    // 尝试添加循环
    bool caught = false;
    try {
        graph.addDependency(1, 3); // x -> y -> z -> x
    } catch (const CycleDetectedException& e) {
        caught = true;
        ASSERT(e.cyclePath.size() > 0);
    }
    ASSERT(caught);
    
    std::cout << "PASSED\n";
}

void test_self_dependency() {
    std::cout << "  test_self_dependency... ";
    DependencyGraph graph;
    
    graph.addNode(1, GraphNodeType::Variable, "x");
    
    bool caught = false;
    try {
        graph.addDependency(1, 1);
    } catch (const CycleDetectedException& e) {
        caught = true;
    }
    ASSERT(caught);
    
    std::cout << "PASSED\n";
}

void test_transitive_dependents() {
    std::cout << "  test_transitive_dependents... ";
    DependencyGraph graph;
    
    // x -> a -> b -> c, a -> d
    graph.addNode(1, GraphNodeType::Variable, "x");
    graph.addNode(2, GraphNodeType::Variable, "a");
    graph.addNode(3, GraphNodeType::Variable, "b");
    graph.addNode(4, GraphNodeType::Variable, "c");
    graph.addNode(5, GraphNodeType::Variable, "d");
    
    graph.addDependency(2, 1);
    graph.addDependency(3, 2);
    graph.addDependency(4, 3);
    graph.addDependency(5, 2);
    
    auto allDeps = graph.getAllDependents(1);
    ASSERT(allDeps.size() == 4);
    
    std::cout << "PASSED\n";
}

void test_expression_evaluation() {
    std::cout << "  test_expression_evaluation... ";
    DependencyGraph graph;
    
    graph.addNode(1, GraphNodeType::Variable, "a");
    graph.addNode(2, GraphNodeType::Variable, "b");
    graph.addNode(3, GraphNodeType::Variable, "c");
    graph.addNode(4, GraphNodeType::Variable, "result");
    
    graph.getNode(1)->value = 10.0;
    graph.getNode(2)->value = 20.0;
    graph.getNode(3)->value = 5.0;
    
    // result = a + b * c = 10 + 20 * 5 = 110
    graph.setExpression(4, "a + b * c");
    graph.addDependency(4, 1);
    graph.addDependency(4, 2);
    graph.addDependency(4, 3);
    
    double result = graph.evaluate(4);
    ASSERT_NEAR(result, 110.0, 0.001);
    
    std::cout << "PASSED\n";
}

void test_expression_parsing() {
    std::cout << "  test_expression_parsing... ";
    DependencyGraph graph;
    
    graph.addNode(1, GraphNodeType::Variable, "x");
    graph.addNode(2, GraphNodeType::Variable, "y");
    graph.addNode(3, GraphNodeType::Variable, "result");
    
    graph.getNode(1)->value = 10.0;
    graph.getNode(2)->value = 20.0;
    
    // setExpression 会自动解析依赖
    graph.setExpression(3, "x + y");
    
    auto deps = graph.getDirectDependencies(3);
    ASSERT(deps.size() == 2);
    
    double result = graph.evaluate(3);
    ASSERT_NEAR(result, 30.0, 0.001);
    
    std::cout << "PASSED\n";
}

void test_complex_expressions() {
    std::cout << "  test_complex_expressions... ";
    DependencyGraph graph;
    
    graph.addNode(1, GraphNodeType::Variable, "a");
    graph.addNode(2, GraphNodeType::Variable, "b");
    graph.addNode(3, GraphNodeType::Variable, "c");
    graph.addNode(4, GraphNodeType::Variable, "result");
    
    graph.getNode(1)->value = 10.0;
    graph.getNode(2)->value = 3.0;
    graph.getNode(3)->value = 2.0;
    
    // (a + b) * c = 26
    graph.setExpression(4, "(a + b) * c");
    graph.addDependency(4, 1);
    graph.addDependency(4, 2);
    graph.addDependency(4, 3);
    
    double result = graph.evaluate(4);
    ASSERT_NEAR(result, 26.0, 0.001);
    
    // a - b / c = 8.5
    graph.getNode(4)->expression = "a - b / c";
    graph.getNode(4)->state = NodeState::Dirty;
    result = graph.evaluate(4);
    ASSERT_NEAR(result, 8.5, 0.001);
    
    std::cout << "PASSED\n";
}

void test_dirty_marking() {
    std::cout << "  test_dirty_marking... ";
    DependencyGraph graph;
    
    // x -> a -> b
    graph.addNode(1, GraphNodeType::Variable, "x");
    graph.addNode(2, GraphNodeType::Variable, "a");
    graph.addNode(3, GraphNodeType::Variable, "b");
    
    graph.addDependency(2, 1);
    graph.addDependency(3, 2);
    
    auto dirty = graph.getDirtyNodes();
    ASSERT(dirty.size() == 3);
    
    graph.recomputeAll();
    dirty = graph.getDirtyNodes();
    ASSERT(dirty.empty());
    
    graph.getNode(1)->value = 100.0;
    graph.markDirty(1);
    
    dirty = graph.getDirtyNodes();
    ASSERT(dirty.size() == 3); // x, a, b 都变脏
    
    graph.refreshDirtyNodes();
    dirty = graph.getDirtyNodes();
    ASSERT(dirty.empty());
    
    std::cout << "PASSED\n";
}

void test_custom_evaluator() {
    std::cout << "  test_custom_evaluator... ";
    DependencyGraph graph;
    
    graph.addNode(1, GraphNodeType::Variable, "x");
    graph.addNode(2, GraphNodeType::Variable, "y");
    graph.addNode(3, GraphNodeType::Variable, "max");
    
    graph.getNode(1)->value = 10.0;
    graph.getNode(2)->value = 20.0;
    
    graph.setEvaluator(3, [](const std::vector<double>& deps) {
        return std::max(deps[0], deps[1]);
    });
    
    graph.addDependency(3, 1);
    graph.addDependency(3, 2);
    
    double result = graph.evaluate(3);
    ASSERT_NEAR(result, 20.0, 0.001);
    
    std::cout << "PASSED\n";
}

void test_component_nodes() {
    std::cout << "  test_component_nodes... ";
    DependencyGraph graph;
    
    graph.addNode(1, GraphNodeType::Variable, "width");
    graph.addNode(2, GraphNodeType::Variable, "height");
    graph.addNode(3, GraphNodeType::Component, "resistor1");
    
    graph.addDependency(3, 1);
    graph.addDependency(3, 2);
    
    graph.recomputeAll();
    
    graph.getNode(1)->value = 100.0;
    graph.markDirty(1);
    
    auto dirty = graph.getDirtyNodes();
    ASSERT(dirty.size() == 2);
    
    std::cout << "PASSED\n";
}

void test_batch_update() {
    std::cout << "  test_batch_update... ";
    DependencyGraph graph;
    
    // x -> a -> c, y -> b -> c
    graph.addNode(1, GraphNodeType::Variable, "x");
    graph.addNode(2, GraphNodeType::Variable, "y");
    graph.addNode(3, GraphNodeType::Variable, "a");
    graph.addNode(4, GraphNodeType::Variable, "b");
    graph.addNode(5, GraphNodeType::Variable, "c");
    
    graph.addDependency(3, 1);
    graph.addDependency(4, 2);
    graph.addDependency(5, 3);
    graph.addDependency(5, 4);
    
    graph.recomputeAll();
    
    graph.getNode(1)->value = 100.0;
    graph.getNode(2)->value = 200.0;
    
    graph.refreshDirtyNodes();
    
    ASSERT(graph.getNode(3)->state == NodeState::Clean);
    ASSERT(graph.getNode(4)->state == NodeState::Clean);
    ASSERT(graph.getNode(5)->state == NodeState::Clean);
    
    std::cout << "PASSED\n";
}

void test_validation() {
    std::cout << "  test_validation... ";
    DependencyGraph graph;
    
    graph.addNode(1, GraphNodeType::Variable, "x");
    graph.addNode(2, GraphNodeType::Variable, "y");
    graph.addNode(3, GraphNodeType::Variable, "z");
    
    graph.addDependency(2, 1);
    graph.addDependency(3, 2);
    
    ASSERT(graph.validate());
    graph.removeNode(1);
    ASSERT(graph.validate());
    
    std::cout << "PASSED\n";
}

void test_remove_dependency() {
    std::cout << "  test_remove_dependency... ";
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
    
    std::cout << "PASSED\n";
}

void test_clear() {
    std::cout << "  test_clear... ";
    DependencyGraph graph;
    
    graph.addNode(1, GraphNodeType::Variable, "x");
    graph.addNode(2, GraphNodeType::Variable, "y");
    graph.addDependency(2, 1);
    
    ASSERT(graph.nodeCount() == 2);
    ASSERT(graph.edgeCount() == 1);
    
    graph.clear();
    
    ASSERT(graph.nodeCount() == 0);
    ASSERT(graph.edgeCount() == 0);
    
    std::cout << "PASSED\n";
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "========================================\n";
    std::cout << "  DependencyGraph Standalone Tests\n";
    std::cout << "========================================\n\n";
    
    int passed = 0;
    int failed = 0;
    
    auto runTest = [&](void (*fn)(), const char* name) {
        try {
            std::cout << name << "... ";
            fn();
            ++passed;
        } catch (const std::exception& e) {
            std::cout << "FAILED: " << e.what() << "\n";
            ++failed;
        }
    };
    
    runTest(test_node_management, "  node_management");
    runTest(test_basic_dependencies, "  basic_dependencies");
    runTest(test_topological_sort, "  topological_sort");
    runTest(test_cycle_detection, "  cycle_detection");
    runTest(test_self_dependency, "  self_dependency");
    runTest(test_transitive_dependents, "  transitive_dependents");
    runTest(test_expression_evaluation, "  expression_evaluation");
    runTest(test_expression_parsing, "  expression_parsing");
    runTest(test_complex_expressions, "  complex_expressions");
    runTest(test_dirty_marking, "  dirty_marking");
    runTest(test_custom_evaluator, "  custom_evaluator");
    runTest(test_component_nodes, "  component_nodes");
    runTest(test_batch_update, "  batch_update");
    runTest(test_validation, "  validation");
    runTest(test_remove_dependency, "  remove_dependency");
    runTest(test_clear, "  clear");
    
    std::cout << "\n========================================\n";
    std::cout << "  Results: " << passed << " passed, " << failed << " failed\n";
    std::cout << "========================================\n";
    
    return failed > 0 ? 1 : 0;
}
