/**
 * @file PCBShapeAttribute.cpp
 * @brief PCB Shape属性实现 - Observer模式实现参数化联动
 */

#include "PCBShapeAttribute.h"
#include "PCBVariablePool.h"
#include <algorithm>

// OpenCASCADE
#include <Geom_RectangularSurface.hxx>
#include <Geom_Circle.hxx>
#include <Geom_Plane.hxx>
#include <TopoDS_Face.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>

namespace FastPCB {

// ============================================================================
// PCBShapeAttribute Implementation
// ============================================================================

// OCAF需要
const Standard_GUID& PCBShapeAttribute::ID() {
    static Standard_GUID guid("A1B2C3D4-E5F6-7890-ABCD-EF1234567890");
    return guid;
}

PCBShapeAttribute::PCBShapeAttribute()
    : isDirty_(false) {
}

PCBShapeAttribute::~PCBShapeAttribute() {
}

void PCBShapeAttribute::Reset() {
    surface_.Nullify();
    curve_.Nullify();
    shapeData_ = ShapeData();
    paramIndices_.clear();
    isDirty_ = false;
}

void PCBShapeAttribute::Paste(const Handle(TDF_Attribute)& into) {
    // 拷贝实现
}

void PCBShapeAttribute::Restore(const Handle(TDF_Attribute)& from) {
    // 恢复实现
}

Handle(TDF_Attribute) PCBShapeAttribute::Copy() const {
    Handle(PCBShapeAttribute) attr = new PCBShapeAttribute();
    attr->shapeData_ = shapeData_;
    attr->paramIndices_ = paramIndices_;
    attr->isDirty_ = isDirty_;
    return attr;
}

Handle(TDF_Attribute) PCBShapeAttribute::Method() const {
    return Copy();
}

// ===================== Shape操作 =====================

void PCBShapeAttribute::setRectangle(double x, double y, double width, double height) {
    shapeData_.type_ = ShapeType::RECTANGLE;
    shapeData_.data_.rect_.x_ = x;
    shapeData_.data_.rect_.y_ = y;
    shapeData_.data_.rect_.width_ = width;
    shapeData_.data_.rect_.height_ = height;
    isDirty_ = true;
}

void PCBShapeAttribute::setCircle(double x, double y, double radius) {
    shapeData_.type_ = ShapeType::CIRCLE;
    shapeData_.data_.circle_.x_ = x;
    shapeData_.data_.circle_.y_ = y;
    shapeData_.data_.circle_.radius_ = radius;
    isDirty_ = true;
}

void PCBShapeAttribute::setPolygon(const std::vector<double>& points) {
    shapeData_.type_ = ShapeType::POLYGON;
    shapeData_.data_.polygon_.pointCount_ = points.size() / 2;
    // 后续实现：存储点数据
    isDirty_ = true;
}

void PCBShapeAttribute::setPath(const std::vector<double>& points, double width) {
    shapeData_.type_ = ShapeType::PATH;
    shapeData_.data_.path_.pointCount_ = points.size() / 2;
    shapeData_.data_.path_.width_ = width;
    // 后续实现：存储点数据
    isDirty_ = true;
}

// ===================== 参数绑定 =====================

void PCBShapeAttribute::bindParameter(uint32_t paramIndex) {
    if (std::find(paramIndices_.begin(), paramIndices_.end(), paramIndex) == paramIndices_.end()) {
        paramIndices_.push_back(paramIndex);
    }
    isDirty_ = true;
}

void PCBShapeAttribute::bindParameters(const std::vector<uint32_t>& indices) {
    for (uint32_t idx : indices) {
        bindParameter(idx);
    }
}

void PCBShapeAttribute::unbindParameter(uint32_t paramIndex) {
    paramIndices_.erase(
        std::remove(paramIndices_.begin(), paramIndices_.end(), paramIndex),
        paramIndices_.end()
    );
}

// ===================== 获取Geom =====================

Handle(Geom_Surface) PCBShapeAttribute::getSurface() {
    if (isDirty_ || surface_.IsNull()) {
        // 需要重建
    }
    return surface_;
}

Handle(Geom_Curve) PCBShapeAttribute::getCurve() {
    if (isDirty_ || curve_.IsNull()) {
        // 需要重建
    }
    return curve_;
}

TopoDS_Shape PCBShapeAttribute::getShape() {
    if (isDirty_) {
        // 重建逻辑
    }
    return TopoDS_Shape();
}

// ===================== IParamObserver =====================

void PCBShapeAttribute::onParameterChanged(uint32_t paramIndex, double newValue) {
    // 检查是否是绑定的参数
    auto it = std::find(paramIndices_.begin(), paramIndices_.end(), paramIndex);
    if (it != paramIndices_.end()) {
        // 更新对应的Shape数据
        updateShapeData(paramIndex, newValue);
        isDirty_ = true;
    }
}

void PCBShapeAttribute::onParametersChanged(const std::unordered_map<uint32_t, double>& changes) {
    for (const auto& pair : changes) {
        onParameterChanged(pair.first, pair.second);
    }
}

// ===================== 重建 =====================

void PCBShapeAttribute::rebuild(VariablePool* pool) {
    if (!pool) return;
    
    switch (shapeData_.type_) {
        case ShapeType::RECTANGLE:
            rebuildRectangle(pool);
            break;
        case ShapeType::CIRCLE:
            rebuildCircle(pool);
            break;
        case ShapeType::POLYGON:
            rebuildPolygon(pool);
            break;
        default:
            break;
    }
    
    isDirty_ = false;
}

void PCBShapeAttribute::rebuildRectangle(VariablePool* pool) {
    double x = shapeData_.data_.rect_.x_;
    double y = shapeData_.data_.rect_.y_;
    double w = shapeData_.data_.rect_.width_;
    double h = shapeData_.data_.rect_.height_;
    
    // 如果绑定了参数，从变量池获取值
    if (!paramIndices_.empty() && pool) {
        if (paramIndices_.size() >= 1) x = pool->getValue(paramIndices_[0]);
        if (paramIndices_.size() >= 2) y = pool->getValue(paramIndices_[1]);
        if (paramIndices_.size() >= 3) w = pool->getValue(paramIndices_[2]);
        if (paramIndices_.size() >= 4) h = pool->getValue(paramIndices_[3]);
    }
    
    // 创建Geom矩形曲面
    try {
        Handle(Geom_Plane) plane = new Geom_Plane(gp::XY());
        surface_ = new Geom_RectangularSurface(plane, x, x + w, y, y + h);
    } catch (...) {
        // 创建失败
    }
}

void PCBShapeAttribute::rebuildCircle(VariablePool* pool) {
    double x = shapeData_.data_.circle_.x_;
    double y = shapeData_.data_.circle_.y_;
    double r = shapeData_.data_.circle_.radius_;
    
    // 如果绑定了参数
    if (!paramIndices_.empty() && pool) {
        if (paramIndices_.size() >= 1) x = pool->getValue(paramIndices_[0]);
        if (paramIndices_.size() >= 2) y = pool->getValue(paramIndices_[1]);
        if (paramIndices_.size() >= 3) r = pool->getValue(paramIndices_[2]);
    }
    
    // 创建Geom圆
    try {
        gp_Circ circle(gp::OX(), r);
        curve_ = new Geom_Circle(circle);
    } catch (...) {
        // 创建失败
    }
}

void PCBShapeAttribute::rebuildPolygon(VariablePool* pool) {
    // 多边形重建
}

void PCBShapeAttribute::updateShapeData(uint32_t paramIndex, double newValue) {
    // 根据参数索引更新Shape数据
    if (paramIndices_.empty()) return;
    
    auto it = std::find(paramIndices_.begin(), paramIndices_.end(), paramIndex);
    if (it == paramIndices_.end()) return;
    
    size_t idx = std::distance(paramIndices_.begin(), it);
    
    switch (shapeData_.type_) {
        case ShapeType::RECTANGLE:
            if (idx == 0) shapeData_.data_.rect_.x_ = newValue;
            else if (idx == 1) shapeData_.data_.rect_.y_ = newValue;
            else if (idx == 2) shapeData_.data_.rect_.width_ = newValue;
            else if (idx == 3) shapeData_.data_.rect_.height_ = newValue;
            break;
        case ShapeType::CIRCLE:
            if (idx == 0) shapeData_.data_.circle_.x_ = newValue;
            else if (idx == 1) shapeData_.data_.circle_.y_ = newValue;
            else if (idx == 2) shapeData_.data_.circle_.radius_ = newValue;
            break;
        default:
            break;
    }
}

// ============================================================================
// ShapeFactory Implementation
// ============================================================================

Handle(PCBShapeAttribute) ShapeFactory::create(const TDF_Label& label, ShapeType type) {
    Handle(PCBShapeAttribute) attr = new PCBShapeAttribute();
    label.AddAttribute(attr);
    return attr;
}

Handle(PCBShapeAttribute) ShapeFactory::createRectangle(const TDF_Label& label,
                                                         double x, double y, 
                                                         double width, double height) {
    Handle(PCBShapeAttribute) attr = create(label, ShapeType::RECTANGLE);
    attr->setRectangle(x, y, width, height);
    return attr;
}

Handle(PCBShapeAttribute) ShapeFactory::createCircle(const TDF_Label& label,
                                                     double x, double y, 
                                                     double radius) {
    Handle(PCBShapeAttribute) attr = create(label, ShapeType::CIRCLE);
    attr->setCircle(x, y, radius);
    return attr;
}

// ============================================================================
// ParamToShapeMap Implementation
// ============================================================================

void ParamToShapeMap::registerBinding(uint32_t paramIndex, 
                                       const Handle(PCBShapeAttribute)& shape) {
    map_[paramIndex].push_back(shape);
}

void ParamToShapeMap::unregisterBinding(uint32_t paramIndex, 
                                         const Handle(PCBShapeAttribute)& shape) {
    auto it = map_.find(paramIndex);
    if (it != map_.end()) {
        it->second.erase(
            std::remove(it->second.begin(), it->second.end(), shape),
            it->second.end()
        );
    }
}

std::vector<Handle(PCBShapeAttribute)> ParamToShapeMap::getDependentShapes(uint32_t paramIndex) const {
    auto it = map_.find(paramIndex);
    if (it != map_.end()) {
        return it->second;
    }
    return std::vector<Handle(PCBShapeAttribute)>();
}

void ParamToShapeMap::refresh(uint32_t paramIndex, double newValue, VariablePool* pool) {
    auto shapes = getDependentShapes(paramIndex);
    for (auto& shape : shapes) {
        shape->onParameterChanged(paramIndex, newValue);
    }
}

void ParamToShapeMap::refreshBatch(const std::unordered_map<uint32_t, double>& changes, 
                                   VariablePool* pool) {
    for (const auto& pair : changes) {
        refresh(pair.first, pair.second, pool);
    }
}

} // namespace FastPCB
