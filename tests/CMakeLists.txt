find_package(Qt5 REQUIRED COMPONENTS Core Widgets Xml Test)

configure_file(${CMAKE_SOURCE_DIR}/example/basic/layout.xml ${CMAKE_BINARY_DIR}/test_layout.xml COPYONLY)

add_executable(test_parser test_parser.cpp)
target_link_libraries(test_parser PRIVATE BroadItem Qt5::Core Qt5::Widgets Qt5::Xml Qt5::Test)

add_executable(test_property_context test_property_context.cpp)
target_link_libraries(test_property_context PRIVATE BroadItem Qt5::Core Qt5::Widgets Qt5::Xml Qt5::Test)

add_executable(test_sized_element test_sized_element.cpp)
target_link_libraries(test_sized_element PRIVATE BroadItem Qt5::Core Qt5::Widgets Qt5::Xml Qt5::Test)

add_executable(test_layout_behavior test_layout_behavior.cpp)
target_link_libraries(test_layout_behavior PRIVATE BroadItem Qt5::Core Qt5::Widgets Qt5::Xml Qt5::Test)

add_executable(test_for_element test_for_element.cpp)
target_link_libraries(test_for_element PRIVATE BroadItem Qt5::Core Qt5::Widgets Qt5::Xml Qt5::Test)

add_executable(test_flatten_children test_flatten_children.cpp)
target_link_libraries(test_flatten_children PRIVATE BroadItem Qt5::Core Qt5::Widgets Qt5::Xml Qt5::Test)

add_executable(test_binding test_binding.cpp)
target_link_libraries(test_binding PRIVATE BroadItem Qt5::Core Qt5::Widgets Qt5::Xml Qt5::Test)

add_executable(test_expression test_expression.cpp)
target_link_libraries(test_expression PRIVATE BroadItem Qt5::Core Qt5::Widgets Qt5::Xml Qt5::Test)

add_executable(test_reactive_binding test_reactive_binding.cpp)
target_link_libraries(test_reactive_binding PRIVATE BroadItem Qt5::Core Qt5::Widgets Qt5::Xml Qt5::Test)

add_executable(test_connection_line test_connection_line.cpp)
target_link_libraries(test_connection_line PRIVATE BroadItem Qt5::Core Qt5::Widgets Qt5::Xml Qt5::Test)

add_executable(test_anchor_decorator test_anchor_decorator.cpp)
target_link_libraries(test_anchor_decorator PRIVATE BroadItem Qt5::Core Qt5::Widgets Qt5::Xml Qt5::Test)

add_executable(test_regression test_regression.cpp)
target_link_libraries(test_regression PRIVATE BroadItem Qt5::Core Qt5::Widgets Qt5::Xml Qt5::Test)

# 黄金镜像采集/校验工具（P4 起注册 ctest：--verify 等价组逐像素比对）
add_executable(golden_render golden/golden_render.cpp)
target_link_libraries(golden_render PRIVATE BroadItem Qt5::Core Qt5::Widgets Qt5::Xml)

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
add_test(NAME test_golden_render COMMAND golden_render --verify ${CMAKE_SOURCE_DIR}/tests/golden/golden WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
