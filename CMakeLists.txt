cmake_minimum_required(VERSION 3.16)
project(BroadItem VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 REQUIRED COMPONENTS Core Widgets Xml)

add_library(BroadItem STATIC
    # core — top-level orchestration
    src/core/BroadItem.cpp
    src/core/LayoutEngine.cpp
    src/core/Frame.cpp
    include/broaditem/core/BroadItem.h
    include/broaditem/core/LayoutEngine.h
    include/broaditem/core/Frame.h

    # context — property context hierarchy
    src/context/QPropertyContext.cpp
    src/context/ItemPropertyContext.cpp
    include/broaditem/context/PropertyContext.h
    include/broaditem/context/MapPropertyContext.h
    include/broaditem/context/ItemPropertyContext.h
    include/broaditem/context/QPropertyContext.h
    include/broaditem/context/LayoutContext.h

    # expression — path-string parser and binding
    src/expression/Expression.cpp
    src/expression/Binding.cpp
    include/broaditem/expression/Expression.h
    include/broaditem/expression/Binding.h

    # element — base element class hierarchy
    src/element/Element.cpp
    src/element/RenderableElement.cpp
    src/element/ControlElement.cpp
    src/element/SizedElement.cpp
    src/element/ContainerElement.cpp
    include/broaditem/element/Element.h
    include/broaditem/element/ControlElement.h
    include/broaditem/element/RenderableElement.h
    include/broaditem/element/SizedElement.h
    include/broaditem/element/BoxModel.h
    include/broaditem/element/ContainerElement.h

    # layout — multi-child layout containers
    src/element/layout/MultiChildContainer.cpp
    src/element/layout/ColumnLayout.cpp
    src/element/layout/RowLayout.cpp
    src/element/layout/GridLayout.cpp
    src/element/layout/CellElement.cpp
    include/broaditem/element/layout/MultiChildContainer.h
    include/broaditem/element/layout/ColumnLayout.h
    include/broaditem/element/layout/RowLayout.h
    include/broaditem/element/layout/GridLayout.h
    include/broaditem/element/layout/CellElement.h

    # control — control-flow elements
    src/element/control/ForElement.cpp
    src/element/control/IfHasElement.cpp
    include/broaditem/element/control/ForElement.h
    include/broaditem/element/control/IfHasElement.h

    # text — text rendering leaf element
    src/element/text/TextElement.cpp
    include/broaditem/element/text/TextElement.h

    # parser — XML layout parsing and registry
    src/parser/XmlLayoutParser.cpp
    src/parser/LayoutRegistry.cpp
    include/broaditem/parser/XmlLayoutParser.h
    include/broaditem/parser/LayoutRegistry.h

    # reactive — property keys and reactive binding
    src/reactive/ReactiveBinding.cpp
    src/reactive/FollowBinding.cpp
    src/reactive/ConnectionLine.cpp
    include/broaditem/reactive/ReactiveProperty.h
    include/broaditem/reactive/ReactiveBinding.h
    include/broaditem/reactive/FollowBinding.h
    include/broaditem/reactive/ConnectionLine.h
)

target_include_directories(BroadItem PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(BroadItem PUBLIC Qt5::Core Qt5::Widgets Qt5::Xml)

# Tests
enable_testing()
add_subdirectory(example)
add_subdirectory(tests)
