find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Xml Test)

configure_file(${CMAKE_SOURCE_DIR}/example/basic/layout.xml ${CMAKE_BINARY_DIR}/test_layout.xml COPYONLY)

add_executable(test_parser test_parser.cpp)
target_link_libraries(test_parser PRIVATE BroadItem Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Xml Qt${QT_VERSION_MAJOR}::Test)

add_executable(test_property_context test_property_context.cpp)
target_link_libraries(test_property_context PRIVATE BroadItem Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Xml Qt${QT_VERSION_MAJOR}::Test)

add_executable(test_sized_element test_sized_element.cpp)
target_link_libraries(test_sized_element PRIVATE BroadItem Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Xml Qt${QT_VERSION_MAJOR}::Test)

add_executable(test_layout_behavior test_layout_behavior.cpp)
target_link_libraries(test_layout_behavior PRIVATE BroadItem Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Xml Qt${QT_VERSION_MAJOR}::Test)

add_executable(test_for_element test_for_element.cpp)
target_link_libraries(test_for_element PRIVATE BroadItem Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Xml Qt${QT_VERSION_MAJOR}::Test)

add_executable(test_flatten_children test_flatten_children.cpp)
target_link_libraries(test_flatten_children PRIVATE BroadItem Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Xml Qt${QT_VERSION_MAJOR}::Test)

add_executable(test_binding test_binding.cpp)
target_link_libraries(test_binding PRIVATE BroadItem Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Xml Qt${QT_VERSION_MAJOR}::Test)

add_executable(test_expression test_expression.cpp)
target_link_libraries(test_expression PRIVATE BroadItem Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Xml Qt${QT_VERSION_MAJOR}::Test)

add_executable(test_reactive_binding test_reactive_binding.cpp)
target_link_libraries(test_reactive_binding PRIVATE BroadItem Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Xml Qt${QT_VERSION_MAJOR}::Test)

add_executable(test_connection_line test_connection_line.cpp)
target_link_libraries(test_connection_line PRIVATE BroadItem Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Xml Qt${QT_VERSION_MAJOR}::Test)

add_executable(test_anchor_decorator test_anchor_decorator.cpp)
target_link_libraries(test_anchor_decorator PRIVATE BroadItem Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Xml Qt${QT_VERSION_MAJOR}::Test)

add_executable(test_regression test_regression.cpp)
target_link_libraries(test_regression PRIVATE BroadItem Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Xml Qt${QT_VERSION_MAJOR}::Test)

add_executable(test_bound_attributes test_bound_attributes.cpp)
target_link_libraries(test_bound_attributes PRIVATE BroadItem Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Xml Qt${QT_VERSION_MAJOR}::Test)

add_executable(test_update_coalescing test_update_coalescing.cpp)
target_link_libraries(test_update_coalescing PRIVATE BroadItem Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Xml Qt${QT_VERSION_MAJOR}::Test)

add_executable(test_diagnostics test_diagnostics.cpp)
target_link_libraries(test_diagnostics PRIVATE BroadItem Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Xml Qt${QT_VERSION_MAJOR}::Test)

# <image> 元素测试：首个 qrc 测试基建（assets/icons.qrc 编入本目标）
if(QT_VERSION_MAJOR EQUAL 6)
    qt6_add_resources(test_image_element_RCC assets/icons.qrc)
else()
    qt5_add_resources(test_image_element_RCC assets/icons.qrc)
endif()
add_executable(test_image_element test_image_element.cpp ${test_image_element_RCC})
target_link_libraries(test_image_element PRIVATE BroadItem Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Xml Qt${QT_VERSION_MAJOR}::Test)

add_executable(test_rect_element test_rect_element.cpp)
target_link_libraries(test_rect_element PRIVATE BroadItem Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Xml Qt${QT_VERSION_MAJOR}::Test)

# 黄金镜像采集/校验工具（P4 起注册 ctest：--verify 等价组逐像素比对）
add_executable(golden_render golden/golden_render.cpp)
target_link_libraries(golden_render PRIVATE BroadItem Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Xml)

add_test(NAME test_parser COMMAND test_parser WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
add_test(NAME test_property_context COMMAND test_property_context WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
add_test(NAME test_sized_element COMMAND test_sized_element WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
add_test(NAME test_layout_behavior COMMAND test_layout_behavior WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
add_test(NAME test_for_element COMMAND test_for_element WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
add_test(NAME test_flatten_children COMMAND test_flatten_children WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
add_test(NAME test_binding COMMAND test_binding WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
add_test(NAME test_expression COMMAND test_expression WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
add_test(NAME test_reactive_binding COMMAND test_reactive_binding WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
add_test(NAME test_connection_line COMMAND test_connection_line WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
add_test(NAME test_anchor_decorator COMMAND test_anchor_decorator WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
add_test(NAME test_regression COMMAND test_regression WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
add_test(NAME test_bound_attributes COMMAND test_bound_attributes WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
add_test(NAME test_update_coalescing COMMAND test_update_coalescing WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
add_test(NAME test_diagnostics COMMAND test_diagnostics WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
add_test(NAME test_image_element COMMAND test_image_element WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
add_test(NAME test_rect_element COMMAND test_rect_element WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

# golden 基线按 Qt 版本分套（Qt6 字体度量/光栅化与 Qt5 有亚像素级漂移，
# 逐像素比对不可跨栈共用）：
# - tests/golden/golden/       Qt5 基线（offscreen QPA 采集）
# - tests/golden/golden-qt6/   Qt6 基线（同法采集，经人工核验渲染正常后入库）
# cocoa 下字体光栅化不确定性会破坏逐像素比对，必须固定 offscreen
# （根因见 .omo/notepads/p0-2-image-element/issues.md）。
if(QT_VERSION_MAJOR EQUAL 6)
    set(GOLDEN_BASELINE_DIR ${CMAKE_SOURCE_DIR}/tests/golden/golden-qt6)
else()
    set(GOLDEN_BASELINE_DIR ${CMAKE_SOURCE_DIR}/tests/golden/golden)
endif()
add_test(NAME test_golden_render COMMAND golden_render --verify ${GOLDEN_BASELINE_DIR} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
set_tests_properties(test_golden_render PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
