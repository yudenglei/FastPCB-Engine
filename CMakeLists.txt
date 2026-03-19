# FastPCB-Engine CMake配置
# 针对800万级器件的内存优化版本

cmake_minimum_required(VERSION 3.15)
project(FastPCB-Engine VERSION 1.0.0 LANGUAGES CXX)

# C++17标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译选项优化
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native -mtune=native")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto")
endif()

# 调试选项
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -fsanitize=address")
endif()

# 查找OpenCASCADE
find_package(OpenCASCADE REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets)

# 源文件
set(SOURCES
    src/PCBRoot.cpp
    src/PCBComponent.cpp
    src/PCBParameter.cpp
    src/PCBVariablePool.cpp
    src/PCBLayerStack.cpp
    src/PCBGeometry.cpp
    src/PCBTrace.cpp
    src/PCBSurface.cpp
    src/PCBVia.cpp
    src/PCBBondWire.cpp
    src/PCBShapeAttribute.cpp
    src/PCBRefPool.cpp
    src/PCBMemoryPool.cpp
    src/PCBDataModel.cpp
)

set(HEADERS
    include/PCBRoot.h
    include/PCBComponent.h
    include/PCBParameter.h
    include/PCBVariablePool.h
    include/PCBLayerStack.h
    include/PCBGeometry.h
    include/PCBTrace.h
    include/PCBSurface.h
    include/PCBVia.h
    include/PCBBondWire.h
    include/PCBShapeAttribute.h
    include/PCBRefPool.h
    include/PCBMemoryPool.h
    include/PCBDataModel.h
    include/PCBConfig.h
)

# 创建库
add_library(FastPCBEngine STATIC ${SOURCES} ${HEADERS})

# 包含目录
target_include_directories(FastPCBEngine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OpenCASCADE_INCLUDE_DIR}
    ${Qt5_INCLUDE_DIRS}
)

# 链接库
target_link_libraries(FastPCBEngine PRIVATE
    ${OpenCASCADE_LIBRARIES}
    Qt5::Core Qt5::Gui Qt5::Widgets
)

# 单元测试
enable_testing()
add_subdirectory(tests)
