/**
 * @file PCBShapeAttribute.h
 * @brief PCB Shape属性 - TDF_Attribute+Observer模式实现参数化联动
 */

#ifndef PCBSHAPEATTRIBUTE_H
#define PCBSHAPEATTRIBUTE_H

#include "PCBConfig.h"
#include "PCBMemoryPool.h"
#include <vector>
#include <string>
#include <functional>
#include <memory>
#include <unordered_map>

// OpenCASCADE includes
#include <TDF_Attribute.hxx>
#include <TDF_Label.hxx>
#include <Handle.hxx>
#include <Geom_Surface.hxx>
#include <Geom_Curve.hxx>
#include <TopoDS_Shape.hxx>

namespace FastPCB {

// 前向声明
class VariablePool;
class ParamObserver;

// ============================================================================
// Shape数据 - 紧凑存储
// ============================================================================

/**
 * @brief 紧凑Shape数据
 * 
 * 优化策略：
 * - 使用union存储不同类型
 * - 延迟实例化Geom对象
 */
struct ShapeData {
    ShapeType type_;
    
    union {
        // 矩形
        struct {
            double x_, y_, width_, height_;
        } rect_;
        
        // 圆形
        struct {
            double x_, y_, radius_;
        } circle_;
        
        // 多边形（紧凑存储）
        struct {
            uint32_t pointCount_;
            uint32_t pointsOffset_;  // 指向点数组的偏移
        } polygon_;
        
        // 路径
        struct {
            uint32_t pointCount_;
            uint32_t pointsOffset_;
            double width_;
        } path_;
    } data_;
    
    ShapeData() : type_(ShapeType::RECTANGLE) {}
};

// ============================================================================
// 参数观察者接口 - Observer模式核心
// ============================================================================

/**
 * @brief 参数观察者接口
 * 
 * 用于实现参数化变量变化时自动刷新Shape
 */
class IParamObserver {
public:
    virtual ~IParamObserver() = default;
    
    /**
     * @brief 参数变化回调
     * @param paramIndex 参数索引
     * @param newValue 新值
     */
    virtual void onParameterChanged(uint32_t paramIndex, double newValue) = 0;
    
    /**
     * @brief 批量参数变化
     * @param changes 参数变化映射
     */
    virtual void onParametersChanged(const std::unordered_map<uint32_t, double>& changes) = 0;
};

// ============================================================================
// 参数化变量（Observable）
// ============================================================================

/**
 * @brief 可观察的参数化变量
 * 
 * 实现Observer模式的被观察者
 */
class ObservableParam {
private:
    uint32_t index_;
    double value_;
    std::vector<IParamObserver*> observers_;
    
public:
    explicit ObservableParam(uint32_t index = 0) : index_(index), value_(0.0) {}
    
    // 设置值并通知所有观察者
    void setValue(double value) {
        if (value_ != value) {
            value_ = value;
            notifyAll();
        }
    }
    
    double getValue() const { return value_; }
    uint32_t getIndex() const { return index_; }
    
    // 添加观察者
    void attach(IParamObserver* observer) {
        observers_.push_back(observer);
    }
    
    // 移除观察者
    void detach(IParamObserver* observer) {
        auto it = std::find(observers_.begin(), observers_.end(), observer);
        if (it != observers_.end()) {
            observers_.erase(it);
        }
    }
    
private:
    void notifyAll() {
        for (auto* observer : observers_) {
            observer->onParameterChanged(index_, value_);
        }
    }
};

// ============================================================================
// PCB Shape属性 - 实现TDF_Attribute
// ============================================================================

/**
 * @brief PCB Shape属性
 * 
 * 核心功能：
 * - 存储几何数据
 * - 关联参数化变量
 * - 参数变化时自动刷新
 */
class PCBShapeAttribute : public TDF_Attribute, public IParamObserver {
private:
    // Shape数据（紧凑存储）
    ShapeData shapeData_;
    
    // 关联的参数索引（用于联动）
    std::vector<uint32_t> paramIndices_;
    
    // 延迟实例化的Geom对象
    Handle(Geom_Surface) surface_;
    Handle(Geom_Curve) curve_;
    
    // 脏标志（需要重新构建）
    bool isDirty_;
    
    // OCAF需要
    static const Standard_GUID& ID();
    
public:
    // OCAF标准宏
    DEFINE_STANDARD_RTTIEXT(PCBShapeAttribute, TDF_Attribute)
    
    /**
     * @brief 构造函数
     */
    PCBShapeAttribute();
    
    /**
     * @brief 析构函数
     */
    virtual ~PCBShapeAttribute();
    
    // ===================== TDF_Attribute接口 =====================
    
    /**
     * @brief 恢复默认
     */
    virtual void Reset() override;
    
    /**
     * @brief 拷贝
     */
    virtual void Paste(const Handle(TDF_Attribute)& into) override;
    
    /**
     * @brief 撤销
     */
    virtual void Restore(const Handle(TDF_Attribute)& from) override;
    
    /**
     * @brief 追加
     */
    virtual Handle(TDF_Attribute) Copy() const override;
    
    /**
     * @brief 方法
     */
    virtual Handle(TDF_Attribute) Method() const override;
    
    // ===================== Shape操作 =====================
    
    /**
     * @brief 设置矩形
     */
    void setRectangle(double x, double y, double width, double height);
    
    /**
     * @brief 设置圆形
     */
    void setCircle(double x, double y, double radius);
    
    /**
     * @brief 设置多边形
     */
    void setPolygon(const std::vector<double>& points);
    
    /**
     * @brief 设置路径
     */
    void setPath(const std::vector<double>& points, double width);
    
    /**
     * @brief 绑定参数（实现联动）
     */
    void bindParameter(uint32_t paramIndex);
    
    /**
     * @brief 批量绑定参数
     */
    void bindParameters(const std::vector<uint32_t>& indices);
    
    /**
     * @brief 解绑参数
     */
    void unbindParameter(uint32_t paramIndex);
    
    /**
     * @brief 获取Geom对象
     */
    Handle(Geom_Surface) getSurface();
    Handle(Geom_Curve) getCurve();
    
    /**
     * @brief 获取TopoDS Shape
     */
    TopoDS_Shape getShape();
    
    // ===================== IParamObserver接口 =====================
    
    /**
     * @brief 参数变化回调
     */
    virtual void onParameterChanged(uint32_t paramIndex, double newValue) override;
    
    /**
     * @brief 批量参数变化
     */
    virtual void onParametersChanged(const std::unordered_map<uint32_t, double>& changes) override;
    
    // ===================== 状态 =====================
    
    /**
     * @brief 标记脏
     */
    void markDirty() { isDirty_ = true; }
    
    /**
     * @brief 清除脏
     */
    void clearDirty() { isDirty_ = false; }
    
    /**
     * @brief 是否脏
     */
    bool isDirty() const { return isDirty_; }
    
    /**
     * @brief 重建Shape（根据参数值）
     */
    void rebuild(VariablePool* pool);
    
    /**
     * @brief 获取Shape类型
     */
    ShapeType getType() const { return shapeData_.type_; }

private:
    /**
     * @brief 重建矩形
     */
    void rebuildRectangle(VariablePool* pool);
    
    /**
     * @brief 重建圆形
     */
    void rebuildCircle(VariablePool* pool);
    
    /**
     * @brief 重建多边形
     */
    void rebuildPolygon(VariablePool* pool);
};

// ============================================================================
// Shape工厂
// ============================================================================

class ShapeFactory {
public:
    /**
     * @brief 创建Shape属性
     */
    static Handle(PCBShapeAttribute) create(const TDF_Label& label, ShapeType type);
    
    /**
     * @brief 创建矩形
     */
    static Handle(PCBShapeAttribute) createRectangle(const TDF_Label& label,
                                                      double x, double y, 
                                                      double width, double height);
    
    /**
     * @brief 创建圆形
     */
    static Handle(PCBShapeAttribute) createCircle(const TDF_Label& label,
                                                    double x, double y, 
                                                    double radius);
};

// ============================================================================
// 参数到Shape的映射 - 快速查找
// ============================================================================

/**
 * @brief 参数到Shape的映射表
 * 
 * 优化：快速查找哪些Shape依赖于某个参数
 */
class ParamToShapeMap {
private:
    // 参数索引 -> Shape列表（反向索引）
    std::unordered_map<uint32_t, std::vector<Handle(PCBShapeAttribute)>> map_;
    
public:
    /**
     * @brief 注册关联
     */
    void registerBinding(uint32_t paramIndex, const Handle(PCBShapeAttribute)& shape);
    
    /**
     * @brief 取消注册
     */
    void unregisterBinding(uint32_t paramIndex, const Handle(PCBShapeAttribute)& shape);
    
    /**
     * @brief 获取依赖某参数的所有Shape
     */
    std::vector<Handle(PCBShapeAttribute)> getDependentShapes(uint32_t paramIndex) const;
    
    /**
     * @brief 批量刷新（参数变化时调用）
     */
    void refresh(uint32_t paramIndex, double newValue, VariablePool* pool);
    
    /**
     * @brief 批量刷新（多个参数变化）
     */
    void refreshBatch(const std::unordered_map<uint32_t, double>& changes, VariablePool* pool);
};

} // namespace FastPCB

#endif // PCBSHAPEATTRIBUTE_H
