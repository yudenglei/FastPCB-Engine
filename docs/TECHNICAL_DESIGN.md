# FastPCB-Engine 技术设计文档

## 1. OCAF Label 架构设计

### 1.1 Label 分配策略

基于OCAF的顺序加载特性，采用**预分配Label段**策略：

`
Root (Label 0)
├── System (Label 1-100)
│   ├── Version (1)
│   ├── Config (2-10)
│   └── Reserved (11-100)
├── Variables (Label 101-1000) ★ 池化数据前置
│   ├── CoordinatePool (101-200)
│   ├── DimensionPool (201-400)
│   └── ExpressionPool (401-1000)
├── LayerStack (Label 1001-2000)
│   ├── SignalLayers (1001-1100)
│   ├── PowerLayers (1101-1200)
│   └── AuxiliaryLayers (1201-2000)
├── Components (Label 2001-100000)
│   ├── ICs (2001-10000)
│   ├── Passives (10001-50000)
│   └── Connectors (50001-100000)
├── Networks (Label 100001-200000)
├── Geometries (Label 200001-1000000)
│   ├── Traces (200001-400000)
│   ├── Surfaces (400001-600000)
│   ├── Vias (600001-800000)
│   └── BondWires (800001-1000000)
└── Annotations (Label 1000001+)
`

### 1.2 为什么需要池化数据前置？

OCAF的Label遍历是深度优先顺序遍历。当加载器件对象时：
1. 先遍历到变量池（Label 101-1000）
2. 此时建立参数化引用关系
3. 后续器件可以通过引用快速获取变量

**关键点：** 池化数据前置确保后续对象加载时能立即建立关系。

---

## 2. 分层数据结构设计

### 2.1 Via（过孔）分层设计

`cpp
// Via的OCAF结构
class Via {
    // 坐标参数（使用Label引用）
    Label xCoord;      // 引用Variables.CoordinatePool中的Label
    Label yCoord;      // 同上
    
    // 尺寸参数
    Label diameter;    // 引用Variables.DimensionPool
    Label drillSize;   // 同上
    
    // 层信息
    Label startLayer;  // 引用LayerStack
    Label endLayer;    // 引用LayerStack
    
    // 网络
    Label net;         // 引用Networks
    
    // 几何数据（分层存储）
    Label padTop;      // 铜皮数据
    Label padBottom;   // 铜皮数据
    Label drill;       // 钻孔数据
};
`

**内存优化：** 使用整数ID代替Reference：
`cpp
struct ViaData {
    int xCoordID;     // 整数ID，而非Label
    int yCoordID;
    int diameterID;
    int startLayerID;
    int netID;
    // ...
};
`

### 2.2 BondWire（键合线）分层设计

`cpp
class BondWire {
    // 端点坐标
    Label startX, startY, startZ;
    Label endX, endY, endZ;
    
    // 曲线参数
    Label curveType;    // 线型（直线/弧线/抛物线）
    Label height;       // 弧高
    
    // 几何数据
    Label surface;      // 曲面数据
    Label centerline;   // 中心线
};
`

### 2.3 Trace（走线）与Surface（铜皮）

参考KLayout的分层方式：

`cpp
class TraceSegment {
    Label startPoint;   // 起点坐标（引用CoordinatePool）
    Label endPoint;     // 终点坐标
    Label width;       // 线宽（引用DimensionPool）
    Label net;          // 网络引用
    Label layer;        // 层引用
    Label path;         // 实际几何路径
};

class Surface {
    Label boundary;     // 边界多边形
    Label holes;        // 挖孔列表
    Label net;          // 网络
    Label layer;        // 层引用
};
`

---

## 3. 参数化变量优化方案

### 3.1 核心问题分析

| 场景 | 内存占用 |
|------|---------|
| 每个坐标创建Label | 约 100 bytes/坐标 |
| 1000万坐标 | 约 1GB |
| 参数化变量引用 | 额外 50 bytes/引用 |

### 3.2 解决方案：Union + 延迟创建

`cpp
// 紧凑的参数值存储
class ParamValue {
private:
    union {
        double doubleVal;      // 未设置变量: 8 bytes
        int varPoolIndex;      // 变量池索引: 4 bytes
    };
    uint16_t flags;            // 标志: 2 bytes
    // 总计: 14 bytes (对齐后16 bytes)
    
public:
    bool isVariable() const { return flags & 0x01; }
    double getDouble() const { return doubleVal; }
    int getVarIndex() const { return varPoolIndex; }
    
    void setDouble(double v) { 
        doubleVal = v; 
        flags &= ~0x01;  // 清除变量标志
    }
    void setVariable(int idx) {
        varPoolIndex = idx;
        flags |= 0x01;   // 设置变量标志
    }
};
`

### 3.3 参数化变量池设计

`cpp
class VariablePool {
    // 预分配的变量存储（避免频繁new）
    std::vector<Variable> pool_;  // 预分配10万个
    
    // 稀疏索引：LabelID -> PoolIndex
    std::unordered_map<int, int> labelToIndex_;
    
public:
    int createVariable(const std::string& name, double value) {
        int idx = pool_.size();
        pool_.push_back({name, value, true});
        return idx;  // 返回池索引
    }
    
    double evaluate(int poolIndex) {
        auto& var = pool_[poolIndex];
        if (var.isExpression) {
            return evaluateExpression(var.expression);
        }
        return var.value;
    }
};
`

---

## 4. Shape 属性与参数化变量联动

### 4.1 TDF_Attribute 设计

`cpp
class PCBShapeAttribute : public TDF_Attribute {
private:
    Handle(Geom_Surface) surface_;
    std::vector<int> paramIndices_;  // 关联的参数池索引
    int shapeType_;  // 0=Rect, 1=Circle, 2=Polygon
    
    // 紧凑的Shape数据
    union ShapeData {
        struct { double x, y, w, h; } rect;    // 矩形
        struct { double x, y, r; } circle;     // 圆形
        struct { double* points; int count; } polygon;  // 多边形
    } data_;
    
public:
    DEFINE_STANDARD_RTTIEXT(PCBShapeAttribute, TDF_Attribute)
    
    // 参数变化回调
    void OnParameterChanged(int paramIndex, double newValue) {
        switch (shapeType_) {
            case 0:  // Rectangle
                if (paramIndex == paramIndices_[0]) data_.rect.x = newValue;
                if (paramIndex == paramIndices_[1]) data_.rect.y = newValue;
                // ... 更新其他维度
                rebuild();
                break;
            // ...
        }
    }
    
    void rebuild() {
        // 根据参数重新构建Shape
        switch (shapeType_) {
            case 0:  // Rectangle
                surface_ = new Geom_RectangularSurface(
                    data_.rect.x, data_.rect.y,
                    data_.rect.w, data_.rect.h
                );
                break;
            // ...
        }
    }
};
`

### 4.2 Observer 模式实现

`cpp
// 参数化变量（Observable）
class ParamVariable : public Standard_Transient {
    std::vector<Handle<PCBShapeAttribute>> observers_;
    
public:
    void SetValue(double value) {
        value_ = value;
        NotifyAll();  // 通知所有Observer
    }
    
    void Attach(Handle<PCBShapeAttribute> obs) {
        observers_.push_back(obs);
    }
};

// 使用示例
void OnParameterChanged(const Handle(ParamVariable)& var, double newValue) {
    // 遍历所有关联的Shape并更新
    for (auto& shape : var->GetObservers()) {
        shape->OnParameterChanged(var->GetIndex(), newValue);
    }
}
`

---

## 5. Reference 节点优化

### 5.1 问题分析

OCAF的Reference会创建额外的Label，内存开销大：
- 每个Reference: ~50 bytes
- 1000万Reference: ~500MB

### 5.2 优化方案

**方案1: 使用整数ID代替Reference**

`cpp
// 原始方式（耗内存）
class Component {
    Label schematicRef;   // 创建额外Label
    Label footprintRef;   // 创建额外Label
    Label netRef;         // 创建额外Label
};

// 优化后
class Component {
    int schematicID_;     // 整数ID
    int footprintID_;
    int netID_;
};
`

**方案2: 池化共享Reference**

`cpp
class RefPool {
    std::unordered_map<std::string, int> nameToID_;
    std::vector<std::string> idToName_;
    
    int getOrCreate(const std::string& name) {
        auto it = nameToID_.find(name);
        if (it != nameToID_.end()) return it->second;
        int id = idToName_.size();
        nameToID_[name] = id;
        idToName_.push_back(name);
        return id;
    }
};
`

**方案3: Lazy Reference**

`cpp
class LazyRef {
    int targetID_;
    bool resolved_;
    Handle(TDF_Label) resolvedLabel_;
    
    Handle(TDF_Label) Resolve() {
        if (!resolved_) {
            resolvedLabel_ = FindLabelByID(targetID_);
            resolved_ = true;
        }
        return resolvedLabel_;
    }
};
`

---

## 6. 内存占用估算

### 6.1 800万器件场景

| 数据类型 | 数量 | 单个大小 | 总内存 |
|----------|------|---------|--------|
| 组件基础数据 | 800万 | 64 bytes | 512 MB |
| 坐标参数 | 2400万 | 16 bytes | 384 MB |
| 尺寸参数 | 1600万 | 16 bytes | 256 MB |
| Shape数据 | 800万 | 128 bytes | 1024 MB |
| Reference池 | 共享 | - | 200 MB |
| 网络数据 | 100万 | 64 bytes | 64 MB |
| 层叠结构 | 100 | 1 KB | 100 KB |
| **总计** | - | - | **~2.4 GB** |

### 6.2 优化空间

- Shape延迟实例化: -500 MB
- 参数延迟创建: -200 MB
- Reference池化: -100 MB

**优化后目标: ~2 GB**

---

## 7. 性能优化策略

### 7.1 加载优化

1. **多线程加载**: 并行加载不同分区
2. **延迟加载**: 仅加载可见区域数据
3. **LOD分级**: 不同细节级别加载不同数据

### 7.2 渲染优化

1. **GPU实例化**: 相同Shape批量渲染
2. **视锥剔除**: 仅渲染可见对象
3. **Shape缓存**: LRU Cache复用Shape

### 7.3 交互优化

1. **增量更新**: 参数变化只更新受影响区域
2. **后台计算**: 参数变化在后台线程重算
3. **批量操作**: 合并多次参数设置