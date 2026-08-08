find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Xml)

configure_file(basic/layout.xml ${CMAKE_CURRENT_BINARY_DIR}/basic.xml COPYONLY)
configure_file(complex/layout.xml ${CMAKE_CURRENT_BINARY_DIR}/complex.xml COPYONLY)
configure_file(playground/layout.xml ${CMAKE_CURRENT_BINARY_DIR}/layout.xml COPYONLY)
configure_file(main_stretch/main_stretch_without.xml ${CMAKE_CURRENT_BINARY_DIR}/main_stretch_without.xml COPYONLY)
configure_file(main_stretch/main_stretch_with.xml ${CMAKE_CURRENT_BINARY_DIR}/main_stretch_with.xml COPYONLY)
configure_file(basic/layout.xml ${CMAKE_CURRENT_BINARY_DIR}/frame_widget.xml COPYONLY)
configure_file(basic/layout.xml ${CMAKE_CURRENT_BINARY_DIR}/frame_image.xml COPYONLY)
configure_file(multi_instance/status_card.xml ${CMAKE_CURRENT_BINARY_DIR}/status_card.xml COPYONLY)
configure_file(multi_instance/process_list.xml ${CMAKE_CURRENT_BINARY_DIR}/process_list.xml COPYONLY)
configure_file(status_panel/status_panel.xml ${CMAKE_CURRENT_BINARY_DIR}/status_panel.xml COPYONLY)
configure_file(badges/breaker.xml ${CMAKE_CURRENT_BINARY_DIR}/breaker.xml COPYONLY)
configure_file(badges/telemetry.xml ${CMAKE_CURRENT_BINARY_DIR}/telemetry.xml COPYONLY)
configure_file(badges/alarm_banner.xml ${CMAKE_CURRENT_BINARY_DIR}/alarm_banner.xml COPYONLY)
configure_file(badges/switch_ports.xml ${CMAKE_CURRENT_BINARY_DIR}/switch_ports.xml COPYONLY)
configure_file(badges/pump.xml ${CMAKE_CURRENT_BINARY_DIR}/pump.xml COPYONLY)

add_executable(basic basic/main.cpp)
target_link_libraries(basic PRIVATE BroadItem Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Xml)

add_executable(complex complex/main.cpp)
target_link_libraries(complex PRIVATE BroadItem Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Xml)

add_executable(main_stretch main_stretch/main.cpp)
target_link_libraries(main_stretch PRIVATE BroadItem Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Xml)

# Playground target for GUI experiments
add_executable(playground playground/main.cpp)
target_link_libraries(playground PRIVATE BroadItem Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Xml)

# Frame examples
add_executable(frame_widget frame_widget/main.cpp)
target_link_libraries(frame_widget PRIVATE BroadItem Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Xml)

add_executable(frame_image frame_image/main.cpp)
target_link_libraries(frame_image PRIVATE BroadItem Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Xml)

# Reactive binding follow example
add_executable(reactive_follow reactive_follow/main.cpp)
target_link_libraries(reactive_follow PRIVATE BroadItem Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Xml)

# Multi-instance isolation example (same layoutId x2 + another layout x1)
add_executable(multi_instance multi_instance/main.cpp)
target_link_libraries(multi_instance PRIVATE BroadItem Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Xml)

# Status panel example (generic bindings + <image> element showcase)
add_executable(status_panel status_panel/main.cpp)
target_link_libraries(status_panel PRIVATE BroadItem Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Xml)

# Badges gallery (README 标牌画廊集中展示)
add_executable(badges badges/main.cpp)
target_link_libraries(badges PRIVATE BroadItem Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Xml)

# 路径语法交互验证工具（Expression / PathSegment）
add_executable(path_tester path_tester/main.cpp)
target_link_libraries(path_tester PRIVATE BroadItem Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Xml)
