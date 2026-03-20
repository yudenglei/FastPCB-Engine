/**
 * @file PCBConfig.h
 * @brief FastPCB引擎配置 - 内存优化核心配置
 */

#ifndef PCBCONFIG_H
#define PCBCONFIG_H

#include <cstdint>

namespace FastPCB {

// ============================================================================
// 内存优化配置 - 针对800万级器件，10GB内存目标
// ============================================================================

// Label预分配配置
namespace LabelConfig {
    // 系统保留Label
    constexpr int SYSTEM_START = 1;
    constexpr int SYSTEM_END = 100;
    
    // 变量池Label（前置！重要优化）
    constexpr int VARIABLES_START = 101;
    constexpr int VARIABLES_END = 1000;
    constexpr int COORD_POOL_SIZE = 100;      // 坐标池大小
    constexpr int DIMENSION_POOL_SIZE = 200;   // 尺寸池大小
    constexpr int EXPRESSION_POOL_SIZE = 600;  // 表达式池大小
    
    // 层叠结构
    constexpr int LAYER_STACK_START = 1001;
    constexpr int LAYER_STACK_END = 2000;
    constexpr int MAX_LAYERS = 100;
    
    // 器件分区
    constexpr int COMPONENTS_START = 2001;
    constexpr int COMPONENTS_END = 100000;
    constexpr int IC_RANGE_START = 2001;
    constexpr int IC_RANGE_END = 10000;
    constexpr int PASSIVE_RANGE_START = 10001;
    constexpr int PASSIVE_RANGE_END = 50000;
    constexpr int CONNECTOR_RANGE_START = 50001;
    constexpr int CONNECTOR_RANGE_END = 100000;
    
    // 网络
    constexpr int NETWORKS_START = 100001;
    constexpr int NETWORKS_END = 200000;
    
    // 几何数据
    constexpr int GEOMETRIES_START = 200001;
    constexpr int GEOMETRIES_END = 1000000;
    constexpr int TRACE_RANGE_START = 200001;
    constexpr int TRACE_RANGE_END = 400000;
    constexpr int SURFACE_RANGE_START = 400001;
    constexpr int SURFACE_RANGE_END = 600000;
    constexpr int VIA_RANGE_START = 600001;
    constexpr int VIA_RANGE_END = 800000;
    constexpr int BONDWIRE_RANGE_START = 800001;
    constexpr int BONDWIRE_RANGE_END = 1000000;
    
    // 标注
    constexpr int ANNOTATIONS_START = 1000001;
}

// 内存池配置
namespace MemoryConfig {
    // 预分配内存池大小（减少malloc调用）
    constexpr size_t PARAM_POOL_SIZE = 1024 * 1024;      // 100万个参数槽位
    constexpr size_t GEOMETRY_POOL_SIZE = 512 * 1024;    // 50万个几何对象
    constexpr size_t STRING_POOL_SIZE = 64 * 1024 * 1024; // 64MB字符串池
    constexpr size_t REF_POOL_SIZE = 256 * 1024;           // 25万个Reference
    
    // LRU缓存配置
    constexpr size_t SHAPE_CACHE_SIZE = 1024;              // Shape缓存数量
    constexpr size_t ATTRIBUTE_CACHE_SIZE = 2048;          // 属性缓存数量
}

// 参数化变量配置
namespace ParameterConfig {
    // 紧凑参数值大小（优化内存）
    constexpr size_t PARAM_VALUE_SIZE = 16;                // Union结构16字节
    constexpr size_t MAX_EXPRESSION_LENGTH = 256;          // 最大表达式长度
    
    // 延迟创建阈值（到这个数量才创建Label）
    constexpr int LAZY_CREATE_THRESHOLD = 10000;
}

// 性能优化配置
namespace PerformanceConfig {
    // 多线程加载
    constexpr int MAX_LOAD_THREADS = 8;
    
    // 批量操作阈值
    constexpr int BATCH_COMMIT_SIZE = 1000;
    constexpr int BATCH_FLUSH_SIZE = 5000;
    
    // 增量更新
    constexpr bool ENABLE_INCREMENTAL_UPDATE = true;
    constexpr bool ENABLE_BACKGROUND_REBUILD = true;
    
    // LOD分级
    constexpr int LOD_LEVELS = 3;
    constexpr double LOD_DISTANCES[] = {1000.0, 5000.0, 10000.0};
}

// Shape类型
enum class ShapeType : uint8_t {
    RECTANGLE = 0,
    CIRCLE = 1,
    POLYGON = 2,
    PATH = 3,
    SURFACE = 4
};

// 器件类型
enum class ComponentType : uint8_t {
    IC = 0,
    RESISTOR = 1,
    CAPACITOR = 2,
    INDUCTOR = 3,
    CONNECTOR = 4,
    VIA = 5,
    BONDWIRE = 6,
    TRACE = 7,
    SURFACE = 8
};

// 参数标志
namespace ParamFlags {
    constexpr uint16_t IS_VARIABLE = 0x01;
    constexpr uint16_t IS_EXPRESSION = 0x02;
    constexpr uint16_t IS_REFERENCED = 0x04;
    constexpr uint16_t IS_DIRTY = 0x08;
    constexpr uint16_t IS_LOCKED = 0x10;
}

} // namespace FastPCB

#endif // PCBCONFIG_H
