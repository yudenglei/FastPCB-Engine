/**
 * @file PCBDesignRule.cpp
 * @brief DRC实现
 */

#include "PCBDesignRule.h"
#include <iostream>
#include <algorithm>
#include <cmath>

namespace FastPCB {

DRCEngine::DRCEngine(PCBDataModel* model)
    : model_(model),
      minTraceWidth_(0.1),      // 0.1mm
      minClearance_(0.1),       // 0.1mm
      minViaPad_(0.2),          // 0.2mm
      minDrill_(0.15),         // 0.15mm
      minAnnularRing_(0.1)    // 0.1mm
{
}

std::vector<DRCViolation> DRCEngine::runAllChecks() {
    std::vector<DRCViolation> violations;
    
    auto v1 = checkTraceWidth();
    violations.insert(violations.end(), v1.begin(), v1.end());
    
    auto v2 = checkClearance();
    violations.insert(violations.end(), v2.begin(), v2.end());
    
    auto v3 = checkViaSize();
    violations.insert(violations.end(), v3.begin(), v3.end());
    
    return violations;
}

std::vector<DRCViolation> DRCEngine::checkTraceWidth() {
    std::vector<DRCViolation> violations;
    
    // 获取所有Trace
    for (uint32_t i = 0; i < model_->getTraceCount(); ++i) {
        auto* trace = model_->getTrace(i);
        if (!trace) continue;
        
        double width = trace->getWidth(model_->getVariablePool());
        
        if (width < minTraceWidth_) {
            violations.emplace_back(
                DRCViolation::MIN_TRACE_WIDTH,
                DRCViolation::ERROR,
                "Trace width " + std::to_string(width) + "mm < " + std::to_string(minTraceWidth_) + "mm",
                trace->getID()
            );
        }
    }
    
    return violations;
}

std::vector<DRCViolation> DRCEngine::checkClearance() {
    std::vector<DRCViolation> violations;
    
    // 简化实现：只检查相邻Trace
    // 实际需要空间索引优化
    
    return violations;
}

std::vector<DRCViolation> DRCEngine::checkViaSize() {
    std::vector<DRCViolation> violations;
    
    for (uint32_t i = 0; i < model_->getViaCount(); ++i) {
        auto* via = model_->getVia(i);
        if (!via) continue;
        
        double padDiameter = via->getPadDiameter(model_->getVariablePool());
        double drillDiameter = via->getDrillDiameter(model_->getVariablePool());
        
        // 检查焊盘
        if (padDiameter < minViaPad_) {
            violations.emplace_back(
                DRCViolation::MIN_VIA_SIZE,
                DRCViolation::ERROR,
                "Via pad " + std::to_string(padDiameter) + "mm < " + std::to_string(minViaPad_) + "mm",
                via->getID()
            );
        }
        
        // 检查钻孔
        if (drillDiameter < minDrill_) {
            violations.emplace_back(
                DRCViolation::MIN_DRILL_SIZE,
                DRCViolation::ERROR,
                "Via drill " + std::to_string(drillDiameter) + "mm < " + std::to_string(minDrill_) + "mm",
                via->getID()
            );
        }
        
        // 检查环宽
        double annularRing = (padDiameter - drillDiameter) / 2.0;
        if (annularRing < minAnnularRing_) {
            violations.emplace_back(
                DRCViolation::ANNULAR_RING,
                DRCViolation::WARNING,
                "Annular ring " + std::to_string(annularRing) + "mm < " + std::to_string(minAnnularRing_) + "mm",
                via->getID()
            );
        }
    }
    
    return violations;
}

std::vector<DRCViolation> DRCEngine::checkNetShorts() {
    std::vector<DRCViolation> violations;
    // 需要根据网络关系检测短路
    return violations;
}

std::string DRCEngine::generateReport(const std::vector<DRCViolation>& violations) {
    std::string report;
    
    int errors = 0, warnings = 0, infos = 0;
    
    for (const auto& v : violations) {
        switch (v.severity_) {
            case DRCViolation::ERROR: errors++; break;
            case DRCViolation::WARNING: warnings++; break;
            case DRCViolation::INFO: infos++; break;
        }
    }
    
    report += "DRC Report\n";
    report += "==========\n";
    report += "Errors: " + std::to_string(errors) + "\n";
    report += "Warnings: " + std::to_string(warnings) + "\n";
    report += "Info: " + std::to_string(infos) + "\n\n";
    
    if (!violations.empty()) {
        report += "Violations:\n";
        for (const auto& v : violations) {
            std::string severity = (v.severity_ == DRCViolation::ERROR) ? "ERROR" : 
                                   (v.severity_ == DRCViolation::WARNING) ? "WARNING" : "INFO";
            report += "[" + severity + "] " + v.message_ + "\n";
        }
    }
    
    return report;
}

} // namespace FastPCB
