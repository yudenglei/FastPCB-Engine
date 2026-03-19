# FastPCB-Engine

高速PCB引擎 - 面向百万级器件的OCAF优化实现

## 项目目标

- 支持800万量级高速PCB设计
- 内存占用控制在10GB以内
- 保留现有OCAF参数化变量机制
- 支持分层数据结构管理

## 核心技术问题解答

### Q1: OCAF的Label加载是顺序的吗？

**答案：是的，OCAF的Label是顺序创建的。**

OCAF的Label ID是按照创建顺序递增的。因此可以**将池化数据放在很靠前的Label下**：
- Label 0-1000: 保留给池化数据
- Label 1001+: 器件对象

### Q2: 分层数据管理

可以采用类似KLayout的分层方式：

`
Root
├── Variables (变量池 - Label 10-100)
├── Layers (层叠结构 - Label 100-200)  
├── Components (器件 - Label 1000+)
├── Networks (网络 - Label 5000+)
└── Geometries (几何数据 - Label 10000+)
    ├── Traces (走线)
    ├── Surfaces (铜皮)
    ├── Vias (过孔)
    └── BondWires (金手指)
`

### Q3: 参数化变量优化（每个坐标创建Label内存太大的问题）

**解决方案：采用Union结构 + 延迟创建**

`cpp
struct ParameterValue {
    union {
        double doubleValue;  // 未设置变量时
        char* varName;       // 设置变量时（字符串指针）
    };
    bool isVariable;
    Label varLabel;  // 指向变量池中的Label
};
`

**关键优化：**
- 不设置变量时只占用8字节（double）
- 设置变量时才创建额外Label
- 批量参数使用紧凑数组

### Q4: Shape属性与参数化变量联动

**使用TDF_Attribute + Observer模式：**

`cpp
class ShapeAttribute : public TDF_Attribute {
    Handle(Geom_Surface) surface;
    std::vector<Handle<ParamVariable>> params;
    
    void OnParameterChanged(const Handle(ParamVariable)& var);
};
`

### Q5: Reference节点优化

Reference节点确实耗内存。**优化方案：**
1. 使用Integer ID代替Reference
2. 池化Reference（多个对象共享）
3. Lazy Reference（仅在需要时解析）

## 内存优化策略

| 优化点 | 策略 |
|--------|------|
| 器件对象 | 紧凑结构，按需加载 |
| 参数化变量 | Union + 延迟创建 |
| Shape缓存 | LRU Cache |
| Reference | ID索引 + 池化 |
| Label组织 | 池化数据前置 |

## 技术栈

- C++17
- Qt 5.13.1  
- OpenCASCADE 7.x (OCAF)