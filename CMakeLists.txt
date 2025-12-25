cmake_minimum_required(VERSION 3.10)
project(FlexTools VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 包含目录
include_directories(${CMAKE_SOURCE_DIR}/include)

# 源文件
set(FLEX_SOURCES
    src/main.cpp
    src/flex_system_info.cpp
    src/flex_hardware_info.cpp
    src/flex_package_info.cpp
    src/flex_log_collector.cpp
)

# 可执行文件
add_executable(flextools ${FLEX_SOURCES})

# 设置可执行文件属性
set_target_properties(flextools PROPERTIES
    OUTPUT_NAME "flextools"
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

# 安装目标
install(TARGETS flextools
    RUNTIME DESTINATION bin
)

# 安装头文件
install(FILES 
    include/flex_common.h
    src/flex_system_info.h
    src/flex_hardware_info.h
    src/flex_package_info.h
    src/flex_log_collector.h
    DESTINATION include/flextools
)

# 安装脚本
install(FILES scripts/install.sh scripts/uninstall.sh
    DESTINATION share/flextools/scripts
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
)

# 安装文档
install(DIRECTORY docs/
    DESTINATION share/doc/flextools
)

# 创建配置包
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/FlexToolsConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

# 生成并安装配置文件
configure_package_config_file(
    "${CMAKE_SOURCE_DIR}/cmake/FlexToolsConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/FlexToolsConfig.cmake"
    INSTALL_DESTINATION lib/cmake/FlexTools
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/FlexToolsConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/FlexToolsConfigVersion.cmake"
    DESTINATION lib/cmake/FlexTools
)

# 导出目标
export(TARGETS flextools
    FILE "${CMAKE_CURRENT_BINARY_DIR}/FlexToolsTargets.cmake"
)

install(EXPORT FlexToolsTargets
    FILE FlexToolsTargets.cmake
    DESTINATION lib/cmake/FlexTools
)
