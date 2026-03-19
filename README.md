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

### 2. 分层数据结构

- Via: PadTop + PadBottom + Drill
- BondWire: Start + End + Curve + Surface
- Trace: Path + Width + Layer
- Surface: Boundary + Holes + Net

### 3. 参数化变量联动

- TDF_Attribute + Observer模式
- 参数变化自动刷新Shape

## 构建

\\\ash
mkdir build
cd build
cmake ..
make
\\\

## 测试

\\\ash
./FastPCBEngine
\\\

## 性能目标

| 器件数量 | 目标内存 |
|---------|---------|
| 100万 | < 2GB |
| 500万 | < 5GB |
| 800万 | < 10GB |

## 文档

- [README](README.md) - 项目概述
- [TECHNICAL_DESIGN.md](docs/TECHNICAL_DESIGN.md) - 详细技术设计
- [OPTIMIZATION.md](docs/OPTIMIZATION.md) - 优化总结

## 许可证

MIT