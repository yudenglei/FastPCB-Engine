/**
 * @file benchmark.cpp
 * @brief 性能基准测试
 */

#include "PCBDataModel.h"
#include "PCBLoader.h"
#include "PCBConfig.h"
#include <iostream>
#include <chrono>
#include <random>
#include <iomanip>

using namespace FastPCB;

void printMemory(uint64_t bytes) {
    std::cout << std::fixed << std::setprecision(2) 
              << bytes / 1024.0 / 1024.0 << " MB" << std::endl;
}

void benchmarkVariablePool(PCBDataModel& model, int count) {
    std::cout << "\n=== Variable Pool Benchmark (" << count << " vars) ===" << std::endl;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    std::vector<uint32_t> indices;
    indices.reserve(count);
    
    for (int i = 0; i < count; ++i) {
        char name[64];
        snprintf(name, sizeof(name), "bench_var_%d", i);
        uint32_t idx = model.createVariable(name, static_cast<double>(i));
        indices.push_back(idx);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Create: " << duration.count() << " ms" << std::endl;
    
    // 随机访问测试
    start = std::chrono::high_resolution_clock::now();
    
    auto* pool = model.getVariablePool();
    double sum = 0;
    for (int i = 0; i < count; ++i) {
        sum += pool->getValue(indices[i]);
    }
    
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Random Read: " << duration.count() << " ms (sum=" << sum << ")" << std::endl;
    
    // 批量更新测试
    start = std::chrono::high_resolution_clock::now();
    
    std::vector<double> values(count);
    for (int i = 0; i < count; ++i) values[i] = static_cast<double>(i * 10);
    pool->setValuesBatch(indices, values);
    
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Batch Write: " << duration.count() << " ms" << std::endl;
    std::cout << "Memory: ";
    printMemory(model.getEstimatedMemory());
}

void benchmarkComponents(PCBDataModel& model, int count) {
    std::cout << "\n=== Component Benchmark (" << count << " components) ===" << std::endl;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    auto ids = model.createComponents(ComponentType::IC, count);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Create: " << duration.count() << " ms" << std::endl;
    std::cout << "Rate: " << count * 1000.0 / duration.count() << " components/sec" << std::endl;
    
    // 随机访问
    start = std::chrono::high_resolution_clock::now();
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, count - 1);
    
    double sum = 0;
    for (int i = 0; i < count; ++i) {
        auto* comp = model.getComponent(dis(gen));
        if (comp) {
            double x, y;
            comp->getPosition(x, y, model.getVariablePool());
            sum += x + y;
        }
    }
    
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Random Access: " << duration.count() << " ms" << std::endl;
    std::cout << "Memory: ";
    printMemory(model.getEstimatedMemory());
}

void benchmarkVias(PCBDataModel& model, int count) {
    std::cout << "\n=== Via Benchmark (" << count << " vias) ===" << std::endl;
    
    PCBLoader loader(&model, 8);
    
    // 准备数据
    std::vector<double> x(count), y(count), padD(count), drill(count);
    std::vector<uint32_t> netIds(count);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> posDis(0, 100000);
    std::uniform_real_distribution<> sizeDis(0.2, 1.0);
    std::uniform_int_distribution<> netDis(1, 1000);
    
    for (int i = 0; i < count; ++i) {
        x[i] = posDis(gen);
        y[i] = posDis(gen);
        padD[i] = sizeDis(gen);
        drill[i] = padD[i] - 0.1;
        netIds[i] = netDis(gen);
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    loader.loadViasBatch(x, y, padD, drill, netIds);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Total: " << duration.count() << " ms" << std::endl;
    std::cout << "Rate: " << count * 1000.0 / duration.count() << " vias/sec" << std::endl;
    std::cout << "Memory: ";
    printMemory(model.getEstimatedMemory());
}

void benchmarkTraces(PCBDataModel& model, int count) {
    std::cout << "\n=== Trace Benchmark (" << count << " traces) ===" << std::endl;
    
    std::vector<std::vector<double>> paths;
    std::vector<double> widths(count);
    std::vector<uint32_t> layerIds(count);
    std::vector<uint32_t> netIds(count);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> posDis(0, 100000);
    std::uniform_real_distribution<> widthDis(0.1, 1.0);
    std::uniform_int_distribution<> layerDis(1, 20);
    std::uniform_int_distribution<> netDis(1, 1000);
    
    for (int i = 0; i < count; ++i) {
        double x = posDis(gen);
        double y = posDis(gen);
        paths.push_back({x, y, x + 1000, y, x + 1000, y + 500, x, y + 500});
        widths[i] = widthDis(gen);
        layerIds[i] = layerDis(gen);
        netIds[i] = netDis(gen);
    }
    
    PCBLoader loader(&model, 8);
    auto start = std::chrono::high_resolution_clock::now();
    
    loader.loadTracesBatch(paths, widths, layerIds, netIds);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Total: " << duration.count() << " ms" << std::endl;
    std::cout << "Rate: " << count * 1000.0 / duration.count() << " traces/sec" << std::endl;
    std::cout << "Memory: ";
    printMemory(model.getEstimatedMemory());
}

void stressTest(PCBDataModel& model, int iterations) {
    std::cout << "\n=== Stress Test (" << iterations << " iterations) ===" << std::endl;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        // 创建Via
        uint32_t viaId = model.createVia();
        auto* via = model.getVia(viaId);
        if (via) {
            via->setPosition(static_cast<double>(i), static_cast<double>(i * 2), 
                            model.getVariablePool());
        }
        
        // 创建Trace
        uint32_t traceId = model.createTrace();
        auto* trace = model.getTrace(traceId);
        if (trace) {
            trace->setPath({static_cast<double>(i), 0, static_cast<double>(i + 100), 0});
            trace->setWidth(0.2, model.getVariablePool());
        }
        
        // 创建Surface
        uint32_t surfId = model.createSurface();
        auto* surf = model.getSurface(surfId);
        if (surf) {
            surf->setBoundary({0, 0, 1000, 0, 1000, 1000, 0, 1000});
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Total: " << duration.count() << " ms" << std::endl;
    std::cout << "Rate: " << iterations * 1000.0 / duration.count() << " ops/sec" << std::endl;
    std::cout << "Memory: ";
    printMemory(model.getEstimatedMemory());
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  FastPCB Engine - Benchmark Suite" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // 创建模型
    PCBDataModel model;
    if (!model.initialize()) {
        std::cerr << "Failed to initialize model!" << std::endl;
        return 1;
    }
    
    std::cout << "\nInitial Memory: ";
    printMemory(model.getEstimatedMemory());
    
    // 基准测试
    benchmarkVariablePool(model, 100000);
    benchmarkComponents(model, 50000);
    benchmarkVias(model, 50000);
    benchmarkTraces(model, 10000);
    stressTest(model, 10000);
    
    // 最终统计
    std::cout << "\n========================================" << std::endl;
    std::cout << "  Final Statistics:" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Components: " << model.getComponentCount() << std::endl;
    std::cout << "Vias: " << model.getViaCount() << std::endl;
    std::cout << "Traces: " << model.getTraceCount() << std::endl;
    std::cout << "Surfaces: " << model.getSurfaceCount() << std::endl;
    std::cout << "Total Memory: ";
    printMemory(model.getEstimatedMemory());
    
    return 0;
}
