find_package(Qt5 REQUIRED COMPONENTS Core Widgets Xml Test)

configure_file(${CMAKE_SOURCE_DIR}/example/basic/test_layout.xml ${CMAKE_BINARY_DIR}/test_layout.xml COPYONLY)

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

add_test(NAME test_parser COMMAND test_parser WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
add_test(NAME test_property_context COMMAND test_property_context WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
add_test(NAME test_sized_element COMMAND test_sized_element WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
add_test(NAME test_layout_behavior COMMAND test_layout_behavior WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
add_test(NAME test_for_element COMMAND test_for_element WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
add_test(NAME test_flatten_children COMMAND test_flatten_children WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
