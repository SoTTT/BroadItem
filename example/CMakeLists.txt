find_package(Qt5 REQUIRED COMPONENTS Core Widgets Xml)

configure_file(basic/layout.xml ${CMAKE_CURRENT_BINARY_DIR}/basic.xml COPYONLY)
configure_file(complex/layout.xml ${CMAKE_CURRENT_BINARY_DIR}/complex.xml COPYONLY)
configure_file(playground/layout.xml ${CMAKE_CURRENT_BINARY_DIR}/layout.xml COPYONLY)
configure_file(main_stretch/main_stretch_without.xml ${CMAKE_CURRENT_BINARY_DIR}/main_stretch_without.xml COPYONLY)
configure_file(main_stretch/main_stretch_with.xml ${CMAKE_CURRENT_BINARY_DIR}/main_stretch_with.xml COPYONLY)
configure_file(basic/layout.xml ${CMAKE_CURRENT_BINARY_DIR}/frame_widget.xml COPYONLY)
configure_file(basic/layout.xml ${CMAKE_CURRENT_BINARY_DIR}/frame_image.xml COPYONLY)
configure_file(multi_instance/status_card.xml ${CMAKE_CURRENT_BINARY_DIR}/status_card.xml COPYONLY)
configure_file(multi_instance/process_list.xml ${CMAKE_CURRENT_BINARY_DIR}/process_list.xml COPYONLY)

add_executable(basic basic/main.cpp)
target_link_libraries(basic PRIVATE BroadItem Qt5::Core Qt5::Widgets Qt5::Xml)

add_executable(complex complex/main.cpp)
target_link_libraries(complex PRIVATE BroadItem Qt5::Core Qt5::Widgets Qt5::Xml)

add_executable(main_stretch main_stretch/main.cpp)
target_link_libraries(main_stretch PRIVATE BroadItem Qt5::Core Qt5::Widgets Qt5::Xml)

# Playground target for GUI experiments
add_executable(playground playground/main.cpp)
target_link_libraries(playground PRIVATE BroadItem Qt5::Core Qt5::Widgets Qt5::Xml)

# Frame examples
add_executable(frame_widget frame_widget/main.cpp)
target_link_libraries(frame_widget PRIVATE BroadItem Qt5::Core Qt5::Widgets Qt5::Xml)

add_executable(frame_image frame_image/main.cpp)
target_link_libraries(frame_image PRIVATE BroadItem Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Xml)

# Reactive binding follow example
add_executable(reactive_follow reactive_follow/main.cpp)
target_link_libraries(reactive_follow PRIVATE BroadItem Qt5::Core Qt5::Widgets Qt5::Xml)

# Multi-instance isolation example (same layoutId x2 + another layout x1)
add_executable(multi_instance multi_instance/main.cpp)
target_link_libraries(multi_instance PRIVATE BroadItem Qt5::Core Qt5::Widgets Qt5::Xml)
