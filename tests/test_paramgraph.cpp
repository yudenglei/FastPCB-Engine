/**
 * @file test_paramgraph.cpp
 * @brief 参数化变量依赖图测试
 */

#include "PCBParamGraph.h"
#include <iostream>
#include <cassert>
#include <vector>

using namespace FastPCB;

void testCreateNodes() {
    std::cout << "Test: Create Nodes..." << std::endl;
    
    ParamGraph graph;
    
    // 创建变量
    uint32_t id1 = graph.createVariable("x", 10.0);
    uint32_t id2 = graph.createVariable("y", 20.0);
    uint32_t id3 = graph.createVariable("z", 30.0);
    
    assert(id1 != id2);
    assert(id2 != id3);
    assert(graph.size() == 3);
    
    // 检查节点值
    auto* node1 = graph.getNode(id1);
    assert(node1 != nullptr);
    assert(node1->getValue() == 10.0);
    
    // 通过名称获取
    auto* nodeX = graph.getNode("x");
    assert(nodeX != nullptr);
    assert(nodeX->getID() == id1);
    
    std::cout << "  PASSED" << std::endl;
}

void testDependencies() {
    std::cout << "Test: Dependencies..." << std::endl;
    
    ParamGraph graph;
    
    // 创建: a = x + y, b = a * 2, c = b + z
    uint32_t idX = graph.createVariable("x", 10.0);
    uint32_t idY = graph.createVariable("y", 20.0);
    uint32_t idA = graph.createVariable("a", 0.0);
    uint32_t idB = graph.createVariable("b", 0.0);
    uint32_t idZ = graph.createVariable("z", 5.0);
    uint32_t idC = graph.createVariable("c", 0.0);
    
    // 添加依赖关系
    graph.addDependency(idA, idX);  // a depends on x
    graph.addDependency(idA, idY);  // a depends on y
    graph.addDependency(idB, idA);  // b depends on a
    graph.addDependency(idC, idB);  // c depends on b
    graph.addDependency(idC, idZ);  // c depends on z
    
    // 检查依赖关系
    auto* nodeA = graph.getNode(idA);
    assert(nodeA->getDependencies().size() == 2);
    assert(nodeA->getDependents().size() == 1);
    
    auto* nodeC = graph.getNode(idC);
    assert(nodeC->getDependencies().size() == 2);  // depends on b and z
    
    std::cout << "  PASSED" << std::endl;
}

void testTopologicalSort() {
    std::cout << "Test: Topological Sort..." << std::endl;
    
    ParamGraph graph;
    
    // 创建依赖链: x -> a -> b -> c
    uint32_t idX = graph.createVariable("x", 10.0);
    uint32_t idA = graph.createVariable("a", 0.0);
    uint32_t idB = graph.createVariable("b", 0.0);
    uint32_t idC = graph.createVariable("c", 0.0);
    
    graph.addDependency(idA, idX);
    graph.addDependency(idB, idA);
    graph.addDependency(idC, idB);
    
    // 获取拓扑排序
    auto order = graph.getTopologicalOrder();
    
    // 检查顺序：x应该在a之前，a在b之前，b在c之前
    auto itX = std::find(order.begin(), order.end(), idX);
    auto itA = std::find(order.begin(), order.end(), idA);
    auto itB = std::find(order.begin(), order.end(), idB);
    auto itC = std::find(order.begin(), order.end(), idC);
    
    assert(itX < itA);
    assert(itA < itB);
    assert(itB < itC);
    
    std::cout << "  PASSED" << std::endl;
}

void testCycleDetection() {
    std::cout << "Test: Cycle Detection..." << std::endl;
    
    ParamGraph graph;
    
    // 创建正常依赖
    uint32_t idX = graph.createVariable("x", 10.0);
    uint32_t idY = graph.createVariable("y", 20.0);
    graph.addDependency(idY, idX);
    
    assert(!graph.hasCycle());
    
    // 添加循环: x depends on y, y depends on x
    graph.addDependency(idX, idY);
    
    assert(graph.hasCycle());
    
    std::cout << "  PASSED" << std::endl;
}

void testIncrementalUpdate() {
    std::cout << "Test: Incremental Update..." << std::endl;
    
    ParamGraph graph;
    
    // 创建: a = x + y
    uint32_t idX = graph.createVariable("x", 10.0);
    uint32_t idY = graph.createVariable("y", 20.0);
    uint32_t idA = graph.createVariable("a", 0.0);
    
    graph.addDependency(idA, idX);
    graph.addDependency(idA, idY);
    
    // 初始计算
    graph.recomputeAll();
    
    auto* nodeA = graph.getNode(idA);
    assert(nodeA->getCachedValue() == 30.0);  // 10 + 20
    
    // 修改x的值
    graph.setValue(idX, 100.0);
    
    // 增量更新
    graph.update(idX);
    
    assert(nodeA->getCachedValue() == 120.0);  // 100 + 20
    
    std::cout << "  PASSED" << std::endl;
}

void testBatchUpdate() {
    std::cout << "Test: Batch Update..." << std::endl;
    
    ParamGraph graph;
    
    // 创建复杂依赖
    uint32_t x = graph.createVariable("x", 10.0);
    uint32_t y = graph.createVariable("y", 20.0);
    uint32_t z = graph.createVariable("z", 30.0);
    uint32_t a = graph.createVariable("a", 0.0);
    uint32_t b = graph.createVariable("b", 0.0);
    uint32_t c = graph.createVariable("c", 0.0);
    
    graph.addDependency(a, x);
    graph.addDependency(a, y);
    graph.addDependency(b, a);
    graph.addDependency(c, a);
    graph.addDependency(c, z);
    
    // 初始计算
    graph.recomputeAll();
    
    assert(graph.getNode(a)->getCachedValue() == 30.0);   // 10 + 20
    assert(graph.getNode(b)->getCachedValue() == 30.0);   // a
    assert(graph.getNode(c)->getCachedValue() == 60.0);   // a + z = 30 + 30
    
    // 批量修改
    graph.setValue(x, 100.0);
    graph.setValue(z, 50.0);
    
    // 批量更新
    graph.updateBatch({x, z});
    
    assert(graph.getNode(a)->getCachedValue() == 120.0);  // 100 + 20
    assert(graph.getNode(b)->getCachedValue() == 120.0);  // a
    assert(graph.getNode(c)->getCachedValue() == 170.0);  // a + z = 120 + 50
    
    std::cout << "  PASSED" << std::endl;
}

void testAffectedNodes() {
    std::cout << "Test: Affected Nodes..." << std::endl;
    
    ParamGraph graph;
    
    // 创建: x -> a -> b -> c 和 a -> d
    uint32_t idX = graph.createVariable("x", 10.0);
    uint32_t idA = graph.createVariable("a", 0.0);
    uint32_t idB = graph.createVariable("b", 0.0);
    uint32_t idC = graph.createVariable("c", 0.0);
    uint32_t idD = graph.createVariable("d", 0.0);
    
    graph.addDependency(idA, idX);
    graph.addDependency(idB, idA);
    graph.addDependency(idC, idB);
    graph.addDependency(idD, idA);
    
    // 获取受影响的节点（从x出发）
    auto affected = graph.getAffectedNodes(idX);
    
    // 应该有 a, b, c, d 受影响
    assert(affected.size() == 4);
    
    std::cout << "  PASSED" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  ParamGraph Unit Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    
    testCreateNodes();
    testDependencies();
    testTopologicalSort();
    testCycleDetection();
    testIncrementalUpdate();
    testBatchUpdate();
    testAffectedNodes();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "  All Tests PASSED!" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}
