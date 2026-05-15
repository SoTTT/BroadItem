find_package(Qt5 REQUIRED COMPONENTS Core Widgets Xml)

configure_file(basic/test_layout.xml ${CMAKE_CURRENT_BINARY_DIR}/test_layout.xml COPYONLY)
configure_file(complex/test_complex.xml ${CMAKE_CURRENT_BINARY_DIR}/test_complex.xml COPYONLY)

add_executable(test_basic basic/test_basic.cpp)
target_link_libraries(test_basic PRIVATE BroadItem Qt5::Core Qt5::Widgets Qt5::Xml)

add_executable(test_complex complex/test_complex.cpp)
target_link_libraries(test_complex PRIVATE BroadItem Qt5::Core Qt5::Widgets Qt5::Xml)

# Empty playground target for user experiments
add_executable(playground playground/main.cpp)
target_link_libraries(playground PRIVATE BroadItem Qt5::Core Qt5::Widgets Qt5::Xml)
