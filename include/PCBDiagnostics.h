/**
 * @file PCBDiagnostics.h
 * @brief 性能监控和诊断
 */

#ifndef PCBDIAGNOSTICS_H
#define PCBDIAGNOSTICS_H

#include "PCBConfig.h"
#include <atomic>
#include <chrono>
#include <vector>
#include <string>
#include <mutex>

namespace FastPCB {

// ============================================================================
// 性能计数器
// ============================================================================

/**
 * @brief 性能计数器
 */
class PerformanceCounter {
private:
    std::string name_;
    std::atomic<uint64_t> count_;
    std::atomic<uint64_t> totalTime_;  // 纳秒
    std::atomic<uint64_t> minTime_;
    std::atomic<uint64_t> maxTime_;
    
public:
    PerformanceCounter(const std::string& name) 
        : name_(name), count_(0), totalTime_(0), 
          minTime_(UINT64_MAX), maxTime_(0) {}
    
    void record(uint64_t elapsedNs) {
        count_++;
        totalTime_ += elapsedNs;
        
        uint64_t currentMin = minTime_.load();
        while (elapsedNs < currentMin && 
               !minTime_.compare_exchange_weak(currentMin, elapsedNs)) {}
        
        uint64_t currentMax = maxTime_.load();
        while (elapsedNs > currentMax && 
               !maxTime_.compare_exchange_weak(currentMax, elapsedNs)) {}
    }
    
    void reset() {
        count_ = 0;
        totalTime_ = 0;
        minTime_ = UINT64_MAX;
        maxTime_ = 0;
    }
    
    uint64_t getCount() const { return count_.load(); }
    uint64_t getTotalTime() const { return totalTime_.load(); }
    uint64_t getMinTime() const { return minTime_.load(); }
    uint64_t getMaxTime() const { return maxTime_.load(); }
    
    double getAvgTime() const {
        uint64_t c = count_.load();
        return c > 0 ? (double)totalTime_.load() / c / 1000.0 : 0;  // 微秒
    }
    
    const std::string& getName() const { return name_; }
};

// ============================================================================
// 性能监控器
// ============================================================================

/**
 * @brief 性能监控器
 */
class PerformanceMonitor {
private:
    static PerformanceMonitor* instance_;
    std::mutex mutex_;
    std::vector<std::unique_ptr<PerformanceCounter>> counters_;
    
    PerformanceMonitor() = default;
    
public:
    static PerformanceMonitor* getInstance() {
        if (!instance_) {
            instance_ = new PerformanceMonitor();
        }
        return instance_;
    }
    
    PerformanceCounter* getCounter(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        for (auto& counter : counters_) {
            if (counter->getName() == name) {
                return counter.get();
            }
        }
        
        auto counter = std::make_unique<PerformanceCounter>(name);
        auto* ptr = counter.get();
        counters_.push_back(std::move(counter));
        return ptr;
    }
    
    void printReport() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::cout << "\n=== Performance Report ===" << std::endl;
        std::cout << std::setw(30) << std::left << "Operation"
                  << std::setw(12) << "Count"
                  << std::setw(15) << "Avg(us)"
                  << std::setw(15) << "Min(us)"
                  << std::setw(15) << "Max(us)" << std::endl;
        std::cout << std::string(87, '-') << std::endl;
        
        for (auto& counter : counters_) {
            std::cout << std::setw(30) << std::left << counter->getName()
                      << std::setw(12) << counter->getCount()
                      << std::setw(15) << std::fixed << std::setprecision(2) << counter->getAvgTime()
                      << std::setw(15) << counter->getMinTime() / 1000.0
                      << std::setw(15) << counter->getMaxTime() / 1000.0 << std::endl;
        }
    }
};

inline PerformanceMonitor* PerformanceMonitor::instance_ = nullptr;

// ============================================================================
// RAII性能计时器
// ============================================================================

/**
 * @brief RAII性能计时器
 */
class ScopedTimer {
private:
    PerformanceCounter* counter_;
    std::chrono::high_resolution_clock::time_point start_;
    
public:
    ScopedTimer(const std::string& name) {
        counter_ = PerformanceMonitor::getInstance()->getCounter(name);
        start_ = std::chrono::high_resolution_clock::now();
    }
    
    ~ScopedTimer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start_).count();
        counter_->record(elapsed);
    }
};

// ============================================================================
// 内存诊断
// ============================================================================

/**
 * @brief 内存诊断
 */
class MemoryDiagnostics {
private:
    struct MemoryStats {
        size_t poolCapacity;
        size_t poolUsed;
        size_t objectCount;
        size_t peakUsage;
    };
    
    std::mutex mutex_;
    std::vector<MemoryStats> history_;
    
public:
    void recordSnapshot(const std::string& name, size_t capacity, size_t used, size_t objects) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        MemoryStats stats;
        stats.poolCapacity = capacity;
        stats.poolUsed = used;
        stats.objectCount = objects;
        
        // 获取峰值
        if (history_.empty() || used > history_.back().peakUsage) {
            stats.peakUsage = used;
        } else {
            stats.peakUsage = history_.back().peakUsage;
        }
        
        history_.push_back(stats);
    }
    
    void printReport() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::cout << "\n=== Memory Diagnostics ===" << std::endl;
        std::cout << std::setw(20) << "Pool"
                  << std::setw(15) << "Capacity"
                  << std::setw(15) << "Used"
                  << std::setw(15) << "Objects"
                  << std::setw(15) << "Peak" << std::endl;
        std::cout << std::string(80, '-') << std::endl;
        
        for (size_t i = 0; i < history_.size(); ++i) {
            const auto& s = history_[i];
            std::cout << "Snapshot " << i
                      << std::setw(11) << ""
                      << std::setw(15) << s.poolCapacity / 1024 / 1024 << "MB"
                      << std::setw(15) << s.poolUsed / 1024 / 1024 << "MB"
                      << std::setw(15) << s.objectCount
                      << std::setw(15) << s.peakUsage / 1024 / 1024 << "MB" << std::endl;
        }
    }
};

// 便捷宏
#define PERF_MONITOR(name) ScopedTimer _timer(name)
#define PERF_COUNTER(name) PerformanceMonitor::getInstance()->getCounter(name)

} // namespace FastPCB

#endif // PCBDIAGNOSTICS_H
