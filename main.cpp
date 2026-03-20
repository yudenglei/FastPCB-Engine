/**
 * @file main.cpp
 * @brief FastPCB引擎测试程序
 */

#include "PCBDataModel.h"
#include "PCBConfig.h"
#include <iostream>
#include <chrono>
#include <thread>

using namespace FastPCB;

void printMemoryUsage(uint64_t bytes) {
    std::cout << "Memory: " << (bytes / 1024.0 / 1024.0) << " MB" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  FastPCB Engine - Memory Optimization Test" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    // 创建数据模型
    PCBDataModel model;
    
    if (!model.initialize()) {
        std::cerr << "Failed to initialize model!" << std::endl;
        return 1;
    }
    
    std::cout << "[1] Model initialized successfully" << std::endl;
    printMemoryUsage(model.getEstimatedMemory());
    std::cout << std::endl;
    
    // 测试创建变量
    std::cout << "[2] Testing variable pool..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    
    // 创建100万个参数化变量
    const int VAR_COUNT = 1000000;
    std::vector<uint32_t> varIndices;
    varIndices.reserve(VAR_COUNT);
    
    for (int i = 0; i < VAR_COUNT; ++i) {
        char name[64];
        snprintf(name, sizeof(name), "var_%d", i);
        uint32_t idx = model.createVariable(name, static_cast<double>(i));
        varIndices.push_back(idx);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "  Created " << VAR_COUNT << " variables in " << duration.count() << "ms" << std::endl;
    printMemoryUsage(model.getEstimatedMemory());
    std::cout << std::endl;
    
    // 测试批量设置变量值
    std::cout << "[3] Testing batch variable update..." << std::endl;
    start = std::chrono::high_resolution_clock::now();
    
    for (int batch = 0; batch < 10; ++batch) {
        std::vector<uint32_t> batchIndices;
        std::vector<double> batchValues;
        
        for (int i = 0; i < 100000; ++i) {
            batchIndices.push_back(varIndices[i]);
            batchValues.push_back(static_cast<double>(i * 10));
        }
        
        auto* pool = model.getVariablePool();
        pool->setValuesBatch(batchIndices, batchValues);
    }
    
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "  Batch update completed in " << duration.count() << "ms" << std::endl;
    printMemoryUsage(model.getEstimatedMemory());
    std::cout << std::endl;
    
    // 测试创建器件
    std::cout << "[4] Testing component creation..." << std::endl;
    start = std::chrono::high_resolution_clock::now();
    
    const int COMP_COUNT = 100000;
    auto compIds = model.createComponents(ComponentType::IC, COMP_COUNT);
    
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "  Created " << COMP_COUNT << " components in " << duration.count() << "ms" << std::endl;
    printMemoryUsage(model.getEstimatedMemory());
    std::cout << std::endl;
    
    // 测试创建Via
    std::cout << "[5] Testing Via creation..." << std::endl;
    start = std::chrono::high_resolution_clock::now();
    
    const int VIA_COUNT = 100000;
    for (int i = 0; i < VIA_COUNT; ++i) {
        uint32_t viaId = model.createVia();
        auto* via = model.getVia(viaId);
        if (via) {
            via->setPosition(static_cast<double>(i), static_cast<double>(i * 2), model.getVariablePool());
            via->setPadDiameter(0.3, model.getVariablePool());
            via->setDrillDiameter(0.2, model.getVariablePool());
        }
    }
    
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "  Created " << VIA_COUNT << " Vias in " << duration.count() << "ms" << std::endl;
    printMemoryUsage(model.getEstimatedMemory());
    std::cout << std::endl;
    
    // 测试创建Trace
    std::cout << "[6] Testing Trace creation..." << std::endl;
    start = std::chrono::high_resolution_clock::now();
    
    const int TRACE_COUNT = 100000;
    for (int i = 0; i < TRACE_COUNT; ++i) {
        uint32_t traceId = model.createTrace();
        auto* trace = model.getTrace(traceId);
        if (trace) {
            std::vector<double> points = {
                static_cast<double>(i), 0.0,
                static_cast<double>(i + 100), 0.0,
                static_cast<double>(i + 100), 50.0,
                static_cast<double>(i + 200), 50.0
            };
            trace->setPath(points);
            trace->setWidth(0.2, model.getVariablePool());
        }
    }
    
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "  Created " << TRACE_COUNT << " Traces in " << duration.count() << "ms" << std::endl;
    printMemoryUsage(model.getEstimatedMemory());
    std::cout << std::endl;
    
    // 最终统计
    std::cout << "========================================" << std::endl;
    std::cout << "  Final Statistics:" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "  Components: " << model.getComponentCount() << std::endl;
    std::cout << "  Vias: " << model.getViaCount() << std::endl;
    std::cout << "  Traces: " << model.getTraceCount() << std::endl;
    std::cout << "  Total Memory: ";
    printMemoryUsage(model.getEstimatedMemory());
    std::cout << std::endl;
    
    // 内存目标检查
    uint64_t mem = model.getEstimatedMemory();
    std::cout << "  Memory Target (1M components): ";
    printMemoryUsage(1000000 * 50);  // 预估
    std::cout << std::endl;
    
    if (mem < 1000000000) {  // < 1GB
        std::cout << "  ✅ EXCELLENT: Memory usage is very low!" << std::endl;
    } else if (mem < 5000000000) {  // < 5GB
        std::cout << "  ✅ GOOD: Memory usage is acceptable" << std::endl;
    } else if (mem < 10000000000) {  // < 10GB
        std::cout << "  ⚠️  WARNING: Memory usage is high but within target" << std::endl;
    } else {
        std::cout << "  ❌ ERROR: Memory usage exceeds 10GB target!" << std::endl;
    }
    
    return 0;
}
