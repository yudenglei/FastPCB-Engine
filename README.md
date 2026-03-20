# FastPCB-Engine

高速PCB引擎 - 面向百万级器件的OCAF优化实现

## 项目目标

- 支持800万量级高速PCB设计
- 内存占用控制在10GB以内（目标2-5GB）
- 保留现有OCAF参数化变量机制
- 支持分层数据结构管理

## 核心技术特性

### 1. 内存优化

- **Union结构参数值**: 16字节紧凑存储 vs 传统100+字节
- **预分配内存池**: 避免频繁malloc/free
- **Reference池化**: 整数ID代替Label引用
- **延迟加载**: 按需实例化

### 2. 参数化依赖图（有向图DAG）

**设计变更**: Observer 模式 → 有向图模式

- 原 Observer 模式是单向的，无法处理表达式变量的**传递依赖**
- 新有向图模式使用 **DAG（有向无环图）** + **拓扑排序**
- 正确处理 `c → b → a` 这种链式驱动

**核心特性**:
- **拓扑排序**: 自动确定计算顺序
- **增量更新**: 只更新受影响节点
- **循环检测**: 防止无限递归
- **表达式求值**: 支持 +, -, *, /, (, ) 运算符
- **组件节点**: 支持器件对象依赖管理
- **脏标记传播**: 变量变化自动驱动下游节点更新

**使用场景**:
```
表达式变量: a = b + c, b = d * 2, c = e - 1
依赖链: d → b → a, e → c → a
当 d 变化时: 通过拓扑排序确定更新顺序 d → b → a
```

### 3. 分层数据结构

- Via: PadTop + PadBottom + Drill
- BondWire: Start + End + Curve + Surface
- Trace: Path + Width + Layer
- Surface: Boundary + Holes + Net

### 4. 性能优化

- 多线程批量加载（8线程）
- LRU缓存
- 批量操作

## 代码结构

```
FastPCB-Engine/
├── CMakeLists.txt          # 构建配置
├── main.cpp               # 主程序
├── benchmark.cpp          # 性能测试
├── include/               # 头文件 (19个)
│   ├── PCBConfig.h         # 配置
│   ├── PCBMemoryPool.h    # 内存池
│   ├── PCBVariablePool.h  # 参数化变量池
│   ├── DependencyGraph.h  # 有向依赖图（新）
│   ├── PCBParamGraph.h    # 参数化依赖图
│   ├── PCBRefPool.h       # Reference池
│   ├── PCBComponent.h     # 器件
│   ├── PCBVia.h           # 过孔
│   ├── PCBBondWire.h     # 键合线
│   ├── PCBTrace.h        # 走线
│   ├── PCBSurface.h      # 铜皮
│   ├── PCBShapeAttribute.h# Shape属性
│   ├── PCBDataModel.h    # 数据模型
│   ├── PCBLoader.h       # 批量加载器
│   ├── PCBGraphNode.h    # 拓扑网络
│   ├── PCBDesignRule.h   # DRC检查
│   ├── PCBSelectionSet.h # 选择集
│   ├── PCBTransaction.h  # 事务管理
│   └── PCBDiagnostics.h  # 性能监控
├── src/                   # 实现 (19个)
└── tests/                 # 单元测试 (5个)
    ├── test_memory_pool.cpp
    ├── test_components.cpp
    ├── test_paramgraph.cpp
    ├── test_dependency_graph.cpp  # 依赖图测试
    └── test_dependency_graph_standalone.cpp  # 独立测试
```

## 构建

```bash
mkdir build
cd build
cmake ..
make
```

## 测试

```bash
# 运行主程序
./FastPCBEngine

# 运行基准测试
./benchmark

# 运行单元测试
./test_memory_pool
./test_components
./test_paramgraph
```

## 内存目标

| 器件数量 | 目标内存 | 预估 |
|---------|---------|------|
| 100万 | < 2GB | ~500MB |
| 500万 | < 5GB | ~2GB |
| 800万 | < 10GB | ~3GB |

## 文档

- [README](README.md) - 项目概述
- [TECHNICAL_DESIGN.md](docs/TECHNICAL_DESIGN.md) - 详细技术设计
- [OPTIMIZATION.md](docs/OPTIMIZATION.md) - 优化总结

## 许可证

MIT
