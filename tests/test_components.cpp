/**
 * @file test_components.cpp
 * @brief 器件和几何对象单元测试
 */

#include "PCBDataModel.h"
#include "PCBConfig.h"
#include <iostream>
#include <cassert>

using namespace FastPCB;

void testVia() {
    std::cout << "Test: PCBVia..." << std::endl;
    
    PCBDataModel model;
    model.initialize();
    
    // 创建Via
    uint32_t viaId = model.createVia();
    auto* via = model.getVia(viaId);
    
    assert(via != nullptr);
    
    // 设置位置
    via->setPosition(100.5, 200.3, model.getVariablePool());
    
    double x, y;
    via->getPosition(x, y, model.getVariablePool());
    assert(std::abs(x - 100.5) < 0.01);
    assert(std::abs(y - 200.3) < 0.01);
    
    // 设置尺寸
    via->setPadDiameter(0.5, model.getVariablePool());
    via->setDrillDiameter(0.3, model.getVariablePool());
    
    assert(std::abs(via->getPadDiameter(model.getVariablePool()) - 0.5) < 0.01);
    assert(std::abs(via->getDrillDiameter(model.getVariablePool()) - 0.3) < 0.01);
    
    // 设置网络
    via->setNet(1);
    assert(via->getNet() == 1);
    
    std::cout << "  PASSED" << std::endl;
}

void testTrace() {
    std::cout << "Test: PCBTrace..." << std::endl;
    
    PCBDataModel model;
    model.initialize();
    
    uint32_t traceId = model.createTrace();
    auto* trace = model.getTrace(traceId);
    
    assert(trace != nullptr);
    
    // 设置路径
    std::vector<double> points = {0, 0, 100, 0, 100, 50, 0, 50};
    trace->setPath(points);
    
    assert(trace->getPointCount() == 4);
    
    // 设置线宽
    trace->setWidth(0.2, model.getVariablePool());
    assert(std::abs(trace->getWidth(model.getVariablePool()) - 0.2) < 0.01);
    
    // 设置层和网络
    trace->setLayer(1);
    trace->setNet(5);
    
    assert(trace->getLayer() == 1);
    assert(trace->getNet() == 5);
    
    std::cout << "  PASSED" << std::endl;
}

void testSurface() {
    std::cout << "Test: PCBSurface..." << std::endl;
    
    PCBDataModel model;
    model.initialize();
    
    uint32_t surfId = model.createSurface();
    auto* surf = model.getSurface(surfId);
    
    assert(surf != nullptr);
    
    // 设置边界
    std::vector<double> boundary = {0, 0, 1000, 0, 1000, 1000, 0, 1000};
    surf->setBoundary(boundary);
    
    assert(surf->getBoundary().size() == 8);
    
    // 添加挖孔
    std::vector<double> hole = {200, 200, 300, 200, 300, 300, 200, 300};
    surf->addHole(hole);
    
    assert(surf->getHoleCount() == 1);
    
    // 计算面积
    surf->calculateArea();
    
    // 1000x1000 = 1000000, 挖孔100x100 = 10000, 面积 = 990000
    double area = surf->getArea();
    assert(area > 900000 && area < 1000000);
    
    std::cout << "  PASSED (area=" << area << ")" << std::endl;
}

void testBondWire() {
    std::cout << "Test: PCBBondWire..." << std::endl;
    
    PCBDataModel model;
    model.initialize();
    
    uint32_t bwId = model.createBondWire(BondWireType::WIRE_BOND);
    auto* bw = model.getBondWire(bwId);
    
    assert(bw != nullptr);
    
    // 设置端点
    bw->setStartPosition(0, 0, 0, model.getVariablePool());
    bw->setEndPosition(1000, 0, 100, model.getVariablePool());
    
    double sx, sy, sz, ex, ey, ez;
    bw->getStartPosition(sx, sy, sz, model.getVariablePool());
    bw->getEndPosition(ex, ey, ez, model.getVariablePool());
    
    assert(std::abs(sx) < 0.01 && std::abs(sy) < 0.01);
    assert(std::abs(ex - 1000) < 0.01 && std::abs(ez - 100) < 0.01);
    
    // 设置线径
    bw->setDiameter(25.0, model.getVariablePool());
    assert(std::abs(bw->getDiameter(model.getVariablePool()) - 25.0) < 0.01);
    
    // 设置网络
    bw->setNet(10);
    assert(bw->getNet() == 10);
    
    std::cout << "  PASSED" << std::endl;
}

void testComponent() {
    std::cout << "Test: PCBComponent..." << std::endl;
    
    PCBDataModel model;
    model.initialize();
    
    uint32_t compId = model.createComponent(ComponentType::IC);
    auto* comp = model.getComponent(compId);
    
    assert(comp != nullptr);
    
    // 设置位置
    comp->setPosition(500.0, 300.0, model.getVariablePool());
    
    double x, y;
    comp->getPosition(x, y, model.getVariablePool());
    assert(std::abs(x - 500.0) < 0.01);
    assert(std::abs(y - 300.0) < 0.01);
    
    // 设置旋转
    comp->setRotation(45.0, model.getVariablePool());
    assert(std::abs(comp->getRotation(model.getVariablePool()) - 45.0) < 0.01);
    
    // 网络
    comp->setNet(3);
    assert(comp->getNet() == 3);
    
    // 标志
    comp->setVisible(true);
    comp->setSelected(false);
    assert(comp->isVisible());
    assert(!comp->isSelected());
    
    std::cout << "  PASSED" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  Component Unit Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    
    testVia();
    testTrace();
    testSurface();
    testBondWire();
    testComponent();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "  All Tests PASSED!" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}
