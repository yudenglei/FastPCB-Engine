# 依赖图设计变更说明

## 变更概述

**从 Observer 模式改为有向图（DAG）模式**

### 为什么需要变更

Observer 模式是**单向**的，无法处理表达式变量的**传递依赖**场景：

```
表达式变量: a = b + c, b = d * 2, c = e - 1
依赖链: d → b → a, e → c → a

当 d 变化时:
- Observer 模式: 只能通知直接观察者 b
- 有向图模式: 通过拓扑排序确定完整更新顺序 d → b → a
```

### 新设计：有向无环图（DAG）

```
┌─────────────────────────────────────────────────────────────┐
│                     依赖图核心概念                            │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  节点类型:                                                  │
│  ┌──────────┐  ┌──────────┐                                │
│  │ Variable │  │Component │                                │
│  │ (表达式) │  │ (器件)   │                                │
│  └──────────┘  └──────────┘                                │
│                                                             │
│  依赖方向:                                                  │
│  dependent -> dependency                                    │
│  (下游节点) -> (上游节点)                                    │
│                                                             │
│  示例: a = b + c                                            │
│  ┌───┐                                                      │
│  │ a │ 依赖 b 和 c                                          │
│  └───┘                                                      │
│   ↓  ↓                                                      │
│  ┌───┐ ┌───┐                                                │
│  │ b │ │ c │                                                │
│  └───┘ └───┘                                                │
│                                                             │
│  边表示: a.dependencies = {b, c}                            │
│         b.dependents = {a}                                  │
│         c.dependents = {a}                                  │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### 核心功能

1. **拓扑排序**: 自动确定计算顺序
   ```cpp
   auto order = graph.topologicalSort();
   // 返回: [d, e, b, c, a] (按依赖顺序)
   ```

2. **增量更新**: 只更新受影响节点
   ```cpp
   graph.markDirty(d);           // 标记 d 为脏
   graph.refreshDirtyNodes();    // 只更新 d 的下游节点
   ```

3. **循环检测**: 防止无限递归
   ```cpp
   // 尝试添加循环依赖会抛出异常
   graph.addDependency(a, d);  // d → b → a → d (成环!)
   // throws CycleDetectedException
   ```

4. **表达式求值**: 支持数学表达式
   ```cpp
   graph.setExpression(a, "b + c * 2");
   double result = graph.evaluate(a);  // 自动计算
   ```

### 使用示例

```cpp
DependencyGraph graph;

// 创建变量节点
graph.addNode(1, GraphNodeType::Variable, "d");
graph.addNode(2, GraphNodeType::Variable, "b");
graph.addNode(3, GraphNodeType::Variable, "a");

// 设置表达式 (自动解析依赖)
graph.setExpression(2, "d * 2");  // b = d * 2
graph.setExpression(3, "b + 5");  // a = b + 5

// 初始计算
graph.recomputeAll();
// b = 0 * 2 = 0, a = 0 + 5 = 5

// 修改 d 的值
graph.getNode(1)->value = 100.0;
graph.markDirty(1);

// 增量更新
graph.refreshDirtyNodes();
// b = 100 * 2 = 200, a = 200 + 5 = 205
```

### 复杂场景：菱形依赖

```
        d
      ↙   ↘
    b       c
      ↘   ↙
        a
```

```cpp
// 创建菱形依赖
graph.addNode(1, GraphNodeType::Variable, "d");
graph.addNode(2, GraphNodeType::Variable, "b");
graph.addNode(3, GraphNodeType::Variable, "c");
graph.addNode(4, GraphNodeType::Variable, "a");

graph.addDependency(2, 1);  // b 依赖 d
graph.addDependency(3, 1);  // c 依赖 d
graph.addDependency(4, 2);  // a 依赖 b
graph.addDependency(4, 3);  // a 依赖 c

// 当 d 变化时，b, c, a 都会被更新
// 拓扑排序确保正确的计算顺序
```

### 与器件集成

```cpp
// 变量
graph.addNode(101, GraphNodeType::Variable, "width");
graph.addNode(102, GraphNodeType::Variable, "height");

// 器件（组件节点）
graph.addNode(201, GraphNodeType::Component, "resistor1");

// 器件依赖变量
graph.addDependency(201, 101);  // resistor1 依赖 width
graph.addDependency(201, 102);  // resistor1 依赖 height

// 当 width 或 height 变化时，resistor1 自动被标记为脏
graph.markDirty(101);
// resistor1 也会被标记为脏
```

## API 参考

### 节点管理
- `addNode(id, type, name, userData)` - 添加节点
- `removeNode(id)` - 移除节点
- `getNode(id)` - 获取节点
- `getNodeByName(name)` - 通过名称获取节点

### 依赖关系
- `addDependency(dependent, dependency)` - 添加依赖
- `removeDependency(dependent, dependency)` - 移除依赖
- `getDirectDependencies(id)` - 获取直接依赖
- `getDirectDependents(id)` - 获取直接依赖者
- `getAllDependents(id)` - 获取所有下游节点

### 表达式
- `setExpression(id, expr)` - 设置表达式
- `getExpression(id)` - 获取表达式
- `evaluate(id)` - 求值

### 更新
- `markDirty(id)` - 标记为脏
- `markClean(id)` - 标记为干净
- `getUpdateOrder(changedNodes)` - 获取更新顺序
- `refreshDirtyNodes()` - 刷新所有脏节点
- `recomputeAll()` - 重新计算所有节点

### 图操作
- `topologicalSort()` - 拓扑排序
- `hasCycle()` - 检测循环
- `findCycle()` - 查找循环路径
- `validate()` - 验证图完整性
- `dumpDot(filename)` - 输出 DOT 文件

## 性能考虑

- **拓扑排序**: O(V + E)，V=节点数, E=边数
- **增量更新**: 只更新受影响的节点，避免全量重算
- **表达式解析**: 缓存解析结果，避免重复解析
- **内存**: 使用紧凑存储，整数ID代替对象引用

## 测试覆盖

测试文件: `tests/test_dependency_graph.cpp`

覆盖场景:
- 节点管理
- 基础依赖关系
- 拓扑排序
- 循环检测
- 自依赖检测
- 传递依赖
- 表达式求值
- 表达式依赖解析
- 复杂表达式
- 脏标记和增量更新
- 自定义求值器
- 组件节点
- 链式传播
- 菱形依赖

运行测试:
```bash
./test_dependency_graph
```
