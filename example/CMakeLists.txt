find_package(Qt5 REQUIRED COMPONENTS Core Widgets Xml)

configure_file(basic/layout.xml ${CMAKE_CURRENT_BINARY_DIR}/basic.xml COPYONLY)
configure_file(complex/layout.xml ${CMAKE_CURRENT_BINARY_DIR}/complex.xml COPYONLY)
configure_file(playground/layout.xml ${CMAKE_CURRENT_BINARY_DIR}/layout.xml COPYONLY)
configure_file(main-stretch/main-stretch-without.xml ${CMAKE_CURRENT_BINARY_DIR}/main-stretch-without.xml COPYONLY)
configure_file(main-stretch/main-stretch-with.xml ${CMAKE_CURRENT_BINARY_DIR}/main-stretch-with.xml COPYONLY)

add_executable(basic basic/main.cpp)
target_link_libraries(basic PRIVATE BroadItem Qt5::Core Qt5::Widgets Qt5::Xml)

add_executable(complex complex/main.cpp)
target_link_libraries(complex PRIVATE BroadItem Qt5::Core Qt5::Widgets Qt5::Xml)

add_executable(main-stretch main-stretch/main.cpp)
target_link_libraries(main-stretch PRIVATE BroadItem Qt5::Core Qt5::Widgets Qt5::Xml)

# Playground target for GUI experiments
add_executable(playground playground/main.cpp)
target_link_libraries(playground PRIVATE BroadItem Qt5::Core Qt5::Widgets Qt5::Xml)
