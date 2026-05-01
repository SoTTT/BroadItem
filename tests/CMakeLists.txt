find_package(Qt5 REQUIRED COMPONENTS Core Widgets Xml Test)

add_executable(test_basic test_basic.cpp)
target_link_libraries(test_basic PRIVATE BroadItem Qt5::Core Qt5::Widgets Qt5::Xml)

add_executable(test_parser test_parser.cpp)
target_link_libraries(test_parser PRIVATE BroadItem Qt5::Core Qt5::Widgets Qt5::Xml Qt5::Test)

add_executable(test_complex test_complex.cpp)
target_link_libraries(test_complex PRIVATE BroadItem Qt5::Core Qt5::Widgets Qt5::Xml)
