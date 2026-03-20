/**
 * @file PCBTransaction.h
 * @brief 事务管理 - 撤销/重做
 */

#ifndef PCBTRANSACTION_H
#define PCBTRANSACTION_H

#include <vector>
#include <memory>
#include <functional>
#include <stack>

namespace FastPCB {

// ============================================================================
// 操作类型
// ============================================================================

enum class OperationType {
    CREATE,
    DELETE,
    MODIFY,
    MOVE,
    ROTATE,
    SCALE,
    PROPERTY_CHANGE
};

// ============================================================================
// 事务操作
// ============================================================================

/**
 * @brief 事务操作
 */
class Operation {
public:
    OperationType type_;
    uint32_t objectId_;
    std::string description_;
    
    // 反向操作
    std::function<void()> undo_;
    std::function<void()> redo_;
    
    Operation(OperationType type, uint32_t objId, const std::string& desc)
        : type_(type), objectId_(objId), description_(desc) {}
    
    void undo() { if (undo_) undo_(); }
    void redo() { if (redo_) redo_(); }
};

// ============================================================================
// 事务管理器
// ============================================================================

/**
 * @brief 事务管理器（撤销/重做）
 */
class TransactionManager {
private:
    std::stack<std::shared_ptr<Operation>> undoStack_;
    std::stack<std::shared_ptr<Operation>> redoStack_;
    
    size_t maxHistory_;
    
public:
    TransactionManager(size_t maxHistory = 100) : maxHistory_(maxHistory) {}
    
    /**
     * @brief 执行操作（并记录）
     */
    void execute(std::shared_ptr<Operation> op) {
        op->redo();
        undoStack_.push(op);
        
        // 清除重做栈
        while (!redoStack_.empty()) {
            redoStack_.pop();
        }
        
        // 限制历史长度
        while (undoStack_.size() > maxHistory_) {
            // 移除最旧的操作
            std::vector<std::shared_ptr<Operation>> temp;
            while (!undoStack_.empty()) {
                temp.push_back(undoStack_.top());
                undoStack_.pop();
            }
            // 移除最旧的（temp的最后一个）
            temp.pop_back();
            // 放回
            for (auto it = temp.rbegin(); it != temp.rend(); ++it) {
                undoStack_.push(*it);
            }
        }
    }
    
    /**
     * @brief 撤销
     */
    bool undo() {
        if (undoStack_.empty()) return false;
        
        auto op = undoStack_.top();
        undoStack_.pop();
        op->undo();
        redoStack_.push(op);
        
        return true;
    }
    
    /**
     * @brief 重做
     */
    bool redo() {
        if (redoStack_.empty()) return false;
        
        auto op = redoStack_.top();
        redoStack_.pop();
        op->redo();
        undoStack_.push(op);
        
        return true;
    }
    
    /**
     * @brief 是否可以撤销
     */
    bool canUndo() const { return !undoStack_.empty(); }
    
    /**
     * @brief 是否可以重做
     */
    bool canRedo() const { return !redoStack_.empty(); }
    
    /**
     * @brief 清除历史
     */
    void clear() {
        while (!undoStack_.empty()) undoStack_.pop();
        while (!redoStack_.empty()) redoStack_.pop();
    }
    
    /**
     * @brief 获取历史大小
     */
    size_t getUndoCount() const { return undoStack_.size(); }
    size_t getRedoCount() const { return redoStack_.size(); }
};

} // namespace FastPCB

#endif // PCBTRANSACTION_H
