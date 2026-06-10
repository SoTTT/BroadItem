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
    src/BroadItem.cpp
    src/QPropertyContext.cpp
    src/ItemPropertyContext.cpp
    src/Expression.cpp
    src/Binding.cpp
    src/XmlLayoutParser.cpp
    src/LayoutRegistry.cpp
    src/LayoutEngine.cpp
    src/elements/Element.cpp
    src/elements/RenderableElement.cpp
    src/elements/ControlElement.cpp
    src/elements/SizedElement.cpp
    src/elements/ContainerElement.cpp
    src/elements/TextElement.cpp
    src/elements/ColumnLayout.cpp
    src/elements/RowLayout.cpp
    src/elements/GridLayout.cpp
    src/elements/MultiChildContainer.cpp
    src/elements/CellElement.cpp
    src/elements/ForElement.cpp
    src/elements/IfHasElement.cpp
    include/broaditem/BroadItem.h
    include/broaditem/PropertyContext.h
    include/broaditem/Expression.h
    include/broaditem/Binding.h
    include/broaditem/MapPropertyContext.h
    include/broaditem/ItemPropertyContext.h
    include/broaditem/QPropertyContext.h
    include/broaditem/RenderableElement.h
    include/broaditem/ControlElement.h
    include/broaditem/SizedElement.h
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
