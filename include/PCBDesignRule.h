/**
 * @file PCBDesignRule.h
 * @brief 设计规则检查 (DRC)
 */

#ifndef PCBDESIGNRULE_H
#define PCBDESIGNRULE_H

#include "PCBConfig.h"
#include "PCBDataModel.h"
#include <vector>
#include <string>

namespace FastPCB {

// ============================================================================
// DRC规则
// ============================================================================

/**
 * @brief DRC违规
 */
struct DRCViolation {
    enum Severity {
        ERROR = 0,
        WARNING = 1,
        INFO = 2
    };
    
    enum Type {
        MIN_TRACE_WIDTH = 0,
        MIN_CLEARANCE = 1,
        MIN_VIA_SIZE = 2,
        MIN_DRILL_SIZE = 3,
        ANNULAR_RING = 4,
        SILK_TO_SOLDER = 5,
        SOLDER_BRIDGE = 6,
        NET_SHORT = 7
    };
    
    Type type_;
    Severity severity_;
    std::string message_;
    uint32_t object1_;
    uint32_t object2_;
    
    DRCViolation(Type t, Severity s, const std::string& msg, uint32_t o1 = 0, uint32_t o2 = 0)
        : type_(t), severity_(s), message_(msg), object1_(o1), object2_(o2) {}
};

// ============================================================================
// DRC规则检查器
// ============================================================================

/**
 * @brief DRC规则检查器
 */
class DRCEngine {
private:
    PCBDataModel* model_;
    
    // 设计规则
    double minTraceWidth_;      // 最小走线宽度 (mm)
    double minClearance_;       // 最小间距 (mm)
    double minViaPad_;         // 最小Via焊盘 (mm)
    double minDrill_;          // 最小钻孔 (mm)
    double minAnnularRing_;    // 最小环宽 (mm)
    
public:
    DRCEngine(PCBDataModel* model);
    
    // ==================== 设置规则 ====================
    
    void setMinTraceWidth(double w) { minTraceWidth_ = w; }
    void setMinClearance(double c) { minClearance_ = c; }
    void setMinViaPad(double p) { minViaPad_ = p; }
    void setMinDrill(double d) { minDrill_ = d; }
    void setMinAnnularRing(double r) { minAnnularRing_ = r; }
    
    // ==================== 执行检查 ====================
    
    /**
     * @brief 运行全部DRC检查
     */
    std::vector<DRCViolation> runAllChecks();
    
    /**
     * @brief 检查走线宽度
     */
    std::vector<DRCViolation> checkTraceWidth();
    
    /**
     * @brief 检查间距
     */
    std::vector<DRCViolation> checkClearance();
    
    /**
     * @brief 检查Via尺寸
     */
    std::vector<DRCViolation> checkViaSize();
    
    /**
     * @brief 检查短路
     */
    std::vector<DRCViolation> checkNetShorts();
    
    /**
     * @brief 生成报告
     */
    std::string generateReport(const std::vector<DRCViolation>& violations);
};

} // namespace FastPCB

#endif // PCBDESIGNRULE_H
