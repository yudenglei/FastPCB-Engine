/**
 * @file PCBSelectionSet.cpp
 * @brief 选集管理实现
 */

#include "PCBSelectionSet.h"

namespace FastPCB {

// ============================================================================
// SelectionSet Implementation
// ============================================================================

SelectionSet SelectionSet::intersect(const SelectionSet& other) const {
    SelectionSet result;
    
    for (uint32_t id : ordered_) {
        if (other.isSelected(id)) {
            result.select(id);
        }
    }
    
    return result;
}

SelectionSet SelectionSet::unite(const SelectionSet& other) const {
    SelectionSet result = *this;
    
    for (uint32_t id : other.ordered_) {
        result.select(id);
    }
    
    return result;
}

SelectionSet SelectionSet::subtract(const SelectionSet& other) const {
    SelectionSet result;
    
    for (uint32_t id : ordered_) {
        if (!other.isSelected(id)) {
            result.select(id);
        }
    }
    
    return result;
}

// ============================================================================
// SelectionManager Implementation
// ============================================================================

void SelectionManager::filterSelection(const SelectionFilter& filter) {
    // 根据过滤器筛选当前选择
    // 后续实现
}

void SelectionManager::moveSelection(double dx, double dy) {
    // 移动所有选中对象
    for (uint32_t id : currentSelection_.getAll()) {
        // 获取对象并移动
    }
}

void SelectionManager::rotateSelection(double angle, double cx, double cy) {
    // 旋转所有选中对象
}

void SelectionManager::scaleSelection(double sx, double sy, double cx, double cy) {
    // 缩放所有选中对象
}

} // namespace FastPCB
