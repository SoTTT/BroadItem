find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Xml Test)

configure_file(${CMAKE_SOURCE_DIR}/example/basic/layout.xml ${CMAKE_BINARY_DIR}/test_layout.xml COPYONLY)

# 普通测试的公共封装：新增一个普通测试只需在下方列表追加一行名字。
# 源文件约定为同名 <name>.cpp，链接库与 ctest 注册方式全部统一。
function(bi_add_plain_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE BroadItem Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Xml Qt${QT_VERSION_MAJOR}::Test)
    add_test(NAME ${name} COMMAND ${name} WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
endfunction()

# 普通测试列表（顺序即 ctest -N 展示顺序）
set(BI_PLAIN_TESTS
    test_parser
    test_property_context
    test_sized_element
    test_layout_behavior
    test_for_element
    test_flatten_children
    test_binding
    test_expression
    test_reactive_binding
    test_connection_line
    test_anchor_decorator
    test_regression
    test_bound_attributes
    test_update_coalescing
    test_diagnostics
)
foreach(test_name IN LISTS BI_PLAIN_TESTS)
    bi_add_plain_test(${test_name})
endforeach()

# <image> 元素测试：首个 qrc 测试基建（assets/icons.qrc 编入本目标）
if(QT_VERSION_MAJOR EQUAL 6)
    qt6_add_resources(test_image_element_RCC assets/icons.qrc)
else()
    qt5_add_resources(test_image_element_RCC assets/icons.qrc)
endif()
bi_add_plain_test(test_image_element)
target_sources(test_image_element PRIVATE ${test_image_element_RCC})

bi_add_plain_test(test_rect_element)

# 黄金镜像采集/校验工具（P4 起注册 ctest：--verify 等价组逐像素比对）
add_executable(golden_render golden/golden_render.cpp)
target_link_libraries(golden_render PRIVATE BroadItem Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Xml)

# golden 基线按 平台×Qt 版本 分套（字体度量/光栅化随平台与 Qt 版本有亚像素级
# 漂移，逐像素比对不可跨栈共用）：
# - tests/golden/golden/             macOS Qt5 基线（offscreen QPA 采集）
# - tests/golden/golden-qt6/         macOS Qt6 基线（同法采集，经人工核验渲染正常后入库）
# - tests/golden/golden-linux/       Linux Qt5 基线（golden-capture CI 采集，
#                                    Qt 5.15.2 + fonts-noto-cjk，人工审阅入库）
# - tests/golden/golden-linux-qt6/   Linux Qt6 基线（同上，Qt 6.8.3）
# 其他平台（Windows 等）暂无基线，不注册本测试。
# cocoa 下字体光栅化不确定性会破坏逐像素比对，必须固定 offscreen
# （根因：基线 PNG 采集于 offscreen 无头渲染路径；cocoa 后端的字体光栅化
# 结果与之不同且随显示环境状态变化，同一代码会 MATCH/DIFF 翻转）。
if(APPLE)
    set(GOLDEN_PLATFORM_SUFFIX "")
elseif(UNIX)
    set(GOLDEN_PLATFORM_SUFFIX "-linux")
endif()
if(DEFINED GOLDEN_PLATFORM_SUFFIX)
    if(QT_VERSION_MAJOR EQUAL 6)
        set(GOLDEN_BASELINE_DIR ${CMAKE_SOURCE_DIR}/tests/golden/golden${GOLDEN_PLATFORM_SUFFIX}-qt6)
    else()
        set(GOLDEN_BASELINE_DIR ${CMAKE_SOURCE_DIR}/tests/golden/golden${GOLDEN_PLATFORM_SUFFIX})
    endif()
    add_test(NAME test_golden_render COMMAND golden_render --verify ${GOLDEN_BASELINE_DIR} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    set_tests_properties(test_golden_render PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()
