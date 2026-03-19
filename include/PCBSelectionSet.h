/**
 * @file PCBSelectionSet.h
 * @brief 选集管理 - 高效选择集
 */

#ifndef PCBSELECTIONSET_H
#define PCBSELECTIONSET_H

#include "PCBConfig.h"
#include <vector>
#include <unordered_set>
#include <memory>

namespace FastPCB {

// ============================================================================
// 选择集
// ============================================================================

/**
 * @brief 选择集
 * 
 * 高效管理选中对象，支持：
 * - 快速添加/移除
 * - 批量操作
 * - 过滤器
 */
class SelectionSet {
private:
    std::unordered_set<uint32_t> selected_;
    std::vector<uint32_t> ordered_;  // 有序列表
    
public:
    /**
     * @brief 添加选中
     */
    void select(uint32_t id) {
        if (selected_.insert(id).second) {
            ordered_.push_back(id);
        }
    }
    
    /**
     * @brief 移除选中
     */
    void deselect(uint32_t id) {
        if (selected_.erase(id) > 0) {
            ordered_.erase(
                std::remove(ordered_.begin(), ordered_.end(), id),
                ordered_.end()
            );
        }
    }
    
    /**
     * @brief 切换选中状态
     */
    void toggle(uint32_t id) {
        if (isSelected(id)) {
            deselect(id);
        } else {
            select(id);
        }
    }
    
    /**
     * @brief 检查是否选中
     */
    bool isSelected(uint32_t id) const {
        return selected_.find(id) != selected_.end();
    }
    
    /**
     * @brief 清除所有选中
     */
    void clear() {
        selected_.clear();
        ordered_.clear();
    }
    
    /**
     * @brief 全选
     */
    void selectAll(const std::vector<uint32_t>& ids) {
        for (uint32_t id : ids) {
            select(id);
        }
    }
    
    /**
     * @brief 批量反选
     */
    void deselectAll(const std::vector<uint32_t>& ids) {
        for (uint32_t id : ids) {
            deselect(id);
        }
    }
    
    /**
     * @brief 获取选中数量
     */
    size_t count() const { return selected_.size(); }
    
    /**
     * @brief 是否为空
     */
    bool empty() const { return selected_.empty(); }
    
    /**
     * @brief 获取所有选中ID
     */
    const std::vector<uint32_t>& getAll() const { return ordered_; }
    
    /**
     * @brief 批量添加
     */
    void addRange(const uint32_t* ids, size_t count) {
        for (size_t i = 0; i < count; ++i) {
            select(ids[i]);
        }
    }
    
    /**
     * @brief 交集
     */
    SelectionSet intersect(const SelectionSet& other) const;
    
    /**
     * @brief 并集
     */
    SelectionSet unite(const SelectionSet& other) const;
    
    /**
     * @brief 差集
     */
    SelectionSet subtract(const SelectionSet& other) const;
};

// ============================================================================
// 选择过滤器
// ============================================================================

/**
 * @brief 选择过滤器
 */
class SelectionFilter {
public:
    enum class FilterType {
        ALL = 0,
        BY_TYPE = 1,
        BY_NET = 2,
        BY_LAYER = 3,
        BY_RECT = 4
    };
    
    FilterType type_;
    std::vector<ComponentType> allowedTypes_;
    uint32_t netId_;
    uint32_t layerId_;
    double rect_[4];  // x1, y1, x2, y2
    
    SelectionFilter() : type_(FilterType::ALL) {}
    
    void setTypeFilter(const std::vector<ComponentType>& types) {
        type_ = FilterType::BY_TYPE;
        allowedTypes_ = types;
    }
    
    void setNetFilter(uint32_t netId) {
        type_ = FilterType::BY_NET;
        netId_ = netId;
    }
    
    void setLayerFilter(uint32_t layerId) {
        type_ = FilterType::BY_LAYER;
        layerId_ = layerId;
    }
    
    void setRectFilter(double x1, double y1, double x2, double y2) {
        type_ = FilterType::BY_RECT;
        rect_[0] = x1; rect_[1] = y1;
        rect_[2] = x2; rect_[3] = y2;
    }
};

// ============================================================================
// 选择管理器
// ============================================================================

/**
 * @brief 选择管理器
 */
class SelectionManager {
private:
    SelectionSet currentSelection_;
    SelectionSet previousSelection_;  // 用于Ctrl+Z
    
public:
    // 当前选择
    SelectionSet& getCurrentSelection() { return currentSelection_; }
    const SelectionSet& getCurrentSelection() const { return currentSelection_; }
    
    // 选择操作
    void select(uint32_t id) { currentSelection_.select(id); }
    void deselect(uint32_t id) { currentSelection_.deselect(id); }
    void toggle(uint32_t id) { currentSelection_.toggle(id); }
    void selectAll(const std::vector<uint32_t>& ids) { currentSelection_.selectAll(ids); }
    void clearSelection() { currentSelection_.clear(); }
    
    // 选择历史
    void saveSelection() { previousSelection_ = currentSelection_; }
    void restoreSelection() { currentSelection_ = previousSelection_; }
    
    // 过滤选择
    void filterSelection(const SelectionFilter& filter);
    
    // 选择变换（移动、旋转、缩放）
    void moveSelection(double dx, double dy);
    void rotateSelection(double angle, double cx, double cy);
    void scaleSelection(double sx, double sy, double cx, double cy);
};

} // namespace FastPCB

#endif // PCBSELECTIONSET_H
