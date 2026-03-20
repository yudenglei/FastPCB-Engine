/**
 * @file test_memory_pool.cpp
 * @brief 内存池单元测试
 */

#include "PCBMemoryPool.h"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace FastPCB;

void testParamValue() {
    std::cout << "Test: ParamValue..." << std::endl;
    
    ParamValue pv;
    
    // 测试设置double值
    pv.setDouble(3.14159);
    assert(!pv.isVariable());
    assert(std::abs(pv.getDouble() - 3.14159) < 0.0001);
    
    // 测试标志
    pv.setVariable(123, false);
    assert(pv.isVariable());
    assert(!pv.isExpression());
    assert(pv.getVarIndex() == 123);
    
    // 测试表达式
    pv.setVariable(456, true);
    assert(pv.isVariable());
    assert(pv.isExpression());
    
    // 测试脏标志
    pv.setDirty(true);
    assert(pv.isDirty());
    pv.setDirty(false);
    assert(!pv.isDirty());
    
    std::cout << "  PASSED" << std::endl;
}

void testVariablePool() {
    std::cout << "Test: VariablePool..." << std::endl;
    
    VariablePool pool;
    
    // 创建变量
    uint32_t idx1 = pool.createOrGet("pi", 3.14159);
    assert(idx1 != UINT32_MAX);
    
    uint32_t idx2 = pool.createOrGet("e", 2.71828);
    assert(idx2 != UINT32_MAX);
    
    // 获取已存在的变量
    uint32_t idx3 = pool.createOrGet("pi", 0);  // 应该返回已有的
    assert(idx3 == idx1);
    
    // 获取值
    double pi = pool.getValue(idx1);
    assert(std::abs(pi - 3.14159) < 0.0001);
    
    // 设置值
    pool.setValue(idx1, 3.14);
    assert(std::abs(pool.getValue(idx1) - 3.14) < 0.0001);
    
    // 批量设置
    std::vector<uint32_t> indices = {idx1, idx2};
    std::vector<double> values = {2.0, 1.5};
    pool.setValuesBatch(indices, values);
    assert(std::abs(pool.getValue(idx1) - 2.0) < 0.0001);
    assert(std::abs(pool.getValue(idx2) - 1.5) < 0.0001);
    
    std::cout << "  PASSED" << std::endl;
}

void testRefPool() {
    std::cout << "Test: RefPool..." << std::endl;
    
    RefPool pool;
    
    // 创建Reference
    uint32_t id1 = pool.getOrCreate("Net1");
    uint32_t id2 = pool.getOrCreate("Net2");
    uint32_t id3 = pool.getOrCreate("Net1");  // 应该返回已有的
    
    assert(id1 != id2);
    assert(id1 == id3);
    
    // 获取名称
    assert(pool.getName(id1) == "Net1");
    assert(pool.getName(id2) == "Net2");
    
    // 获取ID
    assert(pool.getID("Net1") == id1);
    assert(pool.getID("Net2") == id2);
    assert(pool.getID("Net3") == 0);
    
    std::cout << "  PASSED" << std::endl;
}

void testRefCollection() {
    std::cout << "Test: RefCollection..." << std::endl;
    
    RefCollection coll;
    
    // 添加
    coll.add(1);
    coll.add(2);
    coll.add(3);
    assert(coll.size() == 3);
    
    // 重复添加应该忽略
    coll.add(1);
    assert(coll.size() == 3);
    
    // 检查存在
    assert(coll.contains(1));
    assert(coll.contains(2));
    assert(!coll.contains(4));
    
    // 移除
    coll.remove(2);
    assert(coll.size() == 2);
    assert(!coll.contains(2));
    
    // 交集
    RefCollection coll2;
    coll2.add(1);
    coll2.add(3);
    coll2.add(5);
    
    auto result = coll.intersect(coll2);
    assert(result.size() == 2);
    assert(result.contains(1));
    assert(result.contains(3));
    assert(!result.contains(2));
    
    // 并集
    auto result2 = coll.unite(coll2);
    assert(result2.size() == 4);  // 1,2,3,5
    
    std::cout << "  PASSED" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  Memory Pool Unit Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    
    testParamValue();
    testVariablePool();
    testRefPool();
    testRefCollection();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "  All Tests PASSED!" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}
