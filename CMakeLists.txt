cmake_minimum_required(VERSION 3.16)
project(BroadItem VERSION 0.9.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt5/Qt6 双栈：优先 Qt6，未安装时回退 Qt5。
# 下游统一使用 Qt${QT_VERSION_MAJOR}:: 形式的导入目标。
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets Xml)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Xml)

# 源码按 C++11 基线编写（生态位面向 Qt5 时代的老工程）；
# Qt6 头文件硬性要求 C++17，Qt6 构建时标准整体提到 17。
if (QT_VERSION_MAJOR EQUAL 6)
    set(CMAKE_CXX_STANDARD 17)
else ()
    set(CMAKE_CXX_STANDARD 11)
endif ()

add_library(BroadItem STATIC
    # core — 顶层编排
    src/core/BroadItem.cpp
    src/core/LayoutEngine.cpp
    src/core/Frame.cpp
    include/broaditem/core/BroadItem.h
    include/broaditem/core/LayoutEngine.h
    include/broaditem/core/Frame.h
    include/broaditem/core/Node.h

    # context — 属性上下文体系
    src/context/PropertyContext.cpp
    src/context/QPropertyContext.cpp
    src/context/ItemPropertyContext.cpp
    include/broaditem/context/PropertyContext.h
    include/broaditem/context/MapPropertyContext.h
    include/broaditem/context/ItemPropertyContext.h
    include/broaditem/context/QPropertyContext.h
    include/broaditem/context/LayoutContext.h

    # diagnostics — 结构化诊断（错误码、收集器、作用域）
    src/diagnostics/Diagnostics.cpp
    include/broaditem/diagnostics/Diagnostic.h
    include/broaditem/diagnostics/ErrorCollector.h
    include/broaditem/diagnostics/Diagnostics.h

    # compat — Qt5/Qt6 差异点集中地（P1-3 纪律：禁止散落版本分支）
    src/compat/QtCompat.cpp
    include/broaditem/compat/QtCompat.h

    # expression — 路径字符串解析与绑定
    src/expression/Expression.cpp
    src/expression/Binding.cpp
    include/broaditem/expression/Expression.h
    include/broaditem/expression/Binding.h

    # element — 元素基类层级
    src/element/Alignment.cpp
    src/element/Element.cpp
    src/element/RenderableElement.cpp
    src/element/ControlElement.cpp
    src/element/SizedElement.cpp
    src/element/ContainerElement.cpp
    include/broaditem/element/Alignment.h
    include/broaditem/element/Element.h
    include/broaditem/element/ControlElement.h
    include/broaditem/element/RenderableElement.h
    include/broaditem/element/SizedElement.h
    include/broaditem/element/BoxModel.h
    include/broaditem/element/ContainerElement.h

    # layout — 多子布局容器
    src/element/layout/MultiChildContainer.cpp
    src/element/layout/LinearLayoutCommon.cpp
    src/element/layout/LinearLayoutCommon.h
    src/element/layout/ColumnLayout.cpp
    src/element/layout/RowLayout.cpp
    src/element/layout/GridLayout.cpp
    src/element/layout/CellElement.cpp
    include/broaditem/element/layout/MultiChildContainer.h
    include/broaditem/element/layout/ColumnLayout.h
    include/broaditem/element/layout/RowLayout.h
    include/broaditem/element/layout/GridLayout.h
    include/broaditem/element/layout/CellElement.h

    # control — 控制流元素
    src/element/control/ForElement.cpp
    src/element/control/IfElement.cpp
    include/broaditem/element/control/ForElement.h
    include/broaditem/element/control/IfElement.h

    # text — 文本渲染叶子元素
    src/element/text/TextElement.cpp
    include/broaditem/element/text/TextElement.h

    # image — 自计算图片叶子元素
    src/element/image/ImageElement.cpp
    include/broaditem/element/image/ImageElement.h

    # rect — 作者定尺寸纯色块叶子元素
    src/element/rect/RectElement.cpp
    include/broaditem/element/rect/RectElement.h

    # parser — XML 布局解析与注册表
    src/parser/XmlLayoutParser.cpp
    src/parser/LayoutRegistry.cpp
    include/broaditem/parser/XmlLayoutParser.h
    include/broaditem/parser/LayoutRegistry.h

    # reactive — 属性键与响应式绑定
    src/reactive/ReactiveProperty.cpp
    src/reactive/ReactiveBinding.cpp
    src/reactive/FollowBinding.cpp
    src/reactive/ConnectionLine.cpp
    src/reactive/AnchorPoint.cpp
    src/reactive/AnchorDecorator.cpp
    include/broaditem/reactive/ReactiveProperty.h
    include/broaditem/reactive/ReactiveBinding.h
    include/broaditem/reactive/FollowBinding.h
    include/broaditem/reactive/ConnectionLine.h
    include/broaditem/reactive/AnchorPoint.h
    include/broaditem/reactive/AnchorDecorator.h
)

target_include_directories(BroadItem PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party>
    $<INSTALL_INTERFACE:include>
)

# 让下游经 find_package 消费时至少以 C++11 编译（全局 CMAKE_CXX_STANDARD 不进导出目标）。
# Qt6 构建下 Qt6::Core 的 INTERFACE_COMPILE_FEATURES 会自动把标准提到 17。
target_compile_features(BroadItem PUBLIC cxx_std_11)

target_link_libraries(BroadItem PUBLIC Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Xml)

# 供 add_subdirectory / FetchContent 方式消费的下游使用与安装导出一致的命名空间目标。
add_library(BroadItem::BroadItem ALIAS BroadItem)

# ---- 安装与导出（P1-4） ----
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS BroadItem
    EXPORT BroadItemTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/broaditem
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# vendored 单头依赖（tl::optional、mpark/variant，命名空间均已隔离），随项目头文件一同安装。
install(DIRECTORY third_party/tl third_party/mpark
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# 布局结构约束文件，供下游校验 XML 使用。
install(FILES broaditem.xsd
    DESTINATION ${CMAKE_INSTALL_DATADIR}/broaditem
)

install(FILES LICENSE THIRD_PARTY_NOTICES.md
    DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/broaditem
)

set(BroadItem_CMAKE_CONFIG_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/BroadItem)

install(EXPORT BroadItemTargets
    FILE BroadItemTargets.cmake
    NAMESPACE BroadItem::
    DESTINATION ${BroadItem_CMAKE_CONFIG_DIR}
)

# 把构建时选定的 Qt 主版本烧进 Config，下游 find_package(BroadItem) 时自动 find_dependency 同一 Qt 主版本。
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/BroadItemConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/BroadItemConfig.cmake
    INSTALL_DESTINATION ${BroadItem_CMAKE_CONFIG_DIR}
)

# 0.x 阶段按 SameMinorVersion 判定兼容（0.y 系列内 API 稳定，跨 minor 允许调整）。
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/BroadItemConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMinorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/BroadItemConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/BroadItemConfigVersion.cmake
    DESTINATION ${BroadItem_CMAKE_CONFIG_DIR}
)

# 测试
enable_testing()
add_subdirectory(example)
add_subdirectory(tests)
