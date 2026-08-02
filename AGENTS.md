> **写 git commit 和注释必须使用中文。**
> **注释统一使用 Doxygen 风格。**
> **#include 统一使用 <broaditem/module/Header.h> 风格。**

# AGENTS.md — BroadItem

Qt5 C++ 静态库：从 XML 文件加载布局，渲染为 `QGraphicsItem`，支持数据绑定、条件分支（`<if>`）与循环（`<for>`）。

定位：给已被锁定在 QGraphicsView 技术栈里的既有应用提供数据驱动的场景内信息标牌（对标 Qwt 的生存策略——零依赖、静态库、可整体 vendor）。定位结论与演进计划见 `doc/设计调整计划.md`。

## 构建

```bash
cmake -B build -S . && cmake --build build          # Qt5（CMAKE_PREFIX_PATH 指向 qt@5）
cmake -B build-qt6 -S . -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt@6 && cmake --build build-qt6   # Qt6
```

- **Qt5/Qt6 双栈**：`find_package(QT NAMES Qt6 Qt5)` 优先 Qt6、回退 Qt5，链接目标统一 `Qt${QT_VERSION_MAJOR}::`；依赖 Core、Widgets、Xml（测试还需 Test）。版本差异点集中在 `compat/` 模块（P1-3 纪律：禁止散落版本分支），目前有 `variantIsNull()`（Qt6 的 `QVariant::isNull()` 不再传播内含类型的 null 性）与 `domSetContent()`（Qt6.8 起 setContent 旧重载废弃）。
- **已知行为差异**：Qt6 的 XML 解析器更严格——未声明命名空间前缀（`b:content` 无 `xmlns:b`、裸 `:content`）在 Qt5 下"parse 成功但无绑定"，Qt6 下为 BI-P-002 语法错误 parse 失败；两版语义等价（均不产生绑定），测试按 `QT_VERSION_CHECK` 分别断言。
- C++17，开启 AUTOMOC/AUTORCC/AUTOUIC，导出 `compile_commands.json`。
- **安装与下游消费**：`cmake --install <build-dir> --prefix <prefix>` 安装头文件、静态库、`broaditem.xsd`、LICENSE 与 CMake 包配置；导出目标 `BroadItem::BroadItem`（`add_subdirectory` 消费有同名 ALIAS）。`BroadItemConfig.cmake` 烧入构建时的 Qt 主版本并自动 `find_dependency`，下游须用同一 Qt 主版本；`cxx_std_17` 以 PUBLIC compile feature 随目标传导。版本 0.x 阶段按 `SameMinorVersion` 判定兼容，发布以 git tag `vX.Y.Z` 为准。
- 库目标为 `BroadItem`（STATIC）。示例与测试可执行文件分别构建于 `example/` 和 `tests/` 子目录。
- 仓库根目录另有 `cmake-build-debug/`（CLion 的 Ninja 构建目录），`build/`（Qt5）与 `build-qt6/`（Qt6）是 `.gitignore` 认可的标准构建目录。

## 测试

```bash
ctest --test-dir build --output-on-failure      # Qt5：全部 18 个 ctest 条目
ctest --test-dir build-qt6 --output-on-failure  # Qt6：18 个（含金图，基线为 tests/golden/golden-qt6/）
./build/tests/test_parser                       # 运行单个测试套件
```

测试套件（均在 `tests/`，`QTEST_MAIN(Test*)` + 包含自身 `.moc`，链接 `Qt${QT_VERSION_MAJOR}::Test`）：

- `test_parser`、`test_property_context`、`test_sized_element`、`test_layout_behavior`、`test_for_element`、`test_flatten_children`、`test_binding`、`test_expression`、`test_reactive_binding`、`test_connection_line`、`test_anchor_decorator`、`test_regression`、`test_bound_attributes`
- `test_update_coalescing`：P0-4 变更合并（`UpdatePolicy`/`flush()`/守卫位）
- `test_diagnostics`：P1-1 结构化诊断。码表 32 个错误码逐一一个用例 + 行为用例（嵌套 Abort 整文件失败、全收集、运行时模板级去重、静默清单、Default 语义回归）
- `test_image_element`：`<image>` 部件测试，首个 qrc 测试基建（`tests/assets/icons.qrc` 经 `qt5/qt6_add_resources` 编入该目标）
- `test_rect_element`：`<rect>` 部件测试（逐维度 measure、fillsCrossAxis 交叉轴恒填充、显式尺寸豁免与居中、绑定求值与拒绝路径、cell/根退化、BI-P-007）
- `test_golden_render`：黄金镜像校验。`tests/golden/golden_render.cpp` 是采集/校验工具（`--capture <dir>` 按内置 manifest 渲染 PNG；`--verify <dir>` 逐像素比对，等价组不一致则退出码 1）。**该测试固定 `QT_QPA_PLATFORM=offscreen`**（`set_tests_properties`），cocoa 下字体光栅化不确定性会破坏逐像素比对，改金图基建时不得去掉此环境变量。**基线按 Qt 版本分套**：`tests/golden/golden/`（Qt5）与 `tests/golden/golden-qt6/`（Qt6，字体度量/光栅化有亚像素级漂移，逐像素比对不可跨栈共用），`tests/CMakeLists.txt` 按 `QT_VERSION_MAJOR` 指向对应基线；改渲染行为时两套都要重新采集。

注意：

- 示例/测试使用的 XML 布局文件经 `configure_file` 拷入构建目录（见 `example/CMakeLists.txt`、`tests/CMakeLists.txt`）。测试报找不到 XML 时先重新构建。
- 黄金基线 PNG 按 Qt 版本分套：`tests/golden/golden/`（Qt5）、`tests/golden/golden-qt6/`（Qt6），对应布局在 `tests/golden/layouts/`。
- 布局 XML 的结构约束由仓库根目录的 `broaditem.xsd` 描述；新增/修改部件时应同步更新 XSD 与设计文档。

## 示例

```bash
./build/example/basic
./build/example/complex
./build/example/playground        # GUI 手动实验场
./build/example/main_stretch      # 主轴等距拉伸演示
./build/example/frame_widget      # Frame 作为 QWidget 内容
./build/example/frame_image       # Frame 渲染到 QImage
./build/example/reactive_follow   # ReactiveBinding 跟随示例
./build/example/multi_instance    # 多实例隔离可视化验证
./build/example/status_panel      # 监控铭牌（通用绑定 + <image> 集中展示）
./build/example/badges            # README 标牌画廊集中展示（--shot <png> 离屏出图后退出）
```

`example/badges/` 布局集由 `badges` 可执行集中展示；单张 PNG 用 `./build/example/frame_image <布局.xml> <输出.png>` 渲染（存于 `doc/images/badges/`）。`frame_image` 第三参数可指定布局可用宽度，缺省 -1 为内容驱动。

## 架构

四阶段流水线：**Materialize**（模板层 → 实例层 `Node` 树）→ **Measure**（自底向上）→ **Layout**（自顶向下）→ **Render**。

- **模板/实例分离**：模板层 `Element` 树在 `parse()` 后不可变，可安全共享（`LayoutRegistry` 按 layoutId 原样返回 `ElementPtr`，支持多实例共用同一布局）；每实例状态（布局矩形、物化子节点）存于实例层 `Node` 树。
- **控制元素透明**：`<for>` 和 `<if>` 不产生 `Node`，不参与 measure/layout/render。`ColumnLayout`/`RowLayout`/`GridLayout` 在物化时调用 `materializeChildren()`，把控制元素结构性地展开为 0..N 个普通元素实例节点。
- **盒模型**：margin → border → background → padding → content。容器尺寸 = content + 装饰层；拉伸时只有 content 区域扩展。
- **数据上下文**：`LayoutContext` 基于 `QVariantMap`。`BroadItem::setDynamicProperty()` 仅当被绑定属性确实被使用时才触发重布局。变更合并：命中后标脏并按 `UpdatePolicy` 调度——默认 `Coalesced`（`QTimer::singleShot(0)` 合并同一事件循环回合内的多次变更为一次重布局），`Synchronous` 可选；`Frame`/`BroadItem` 均有 `flush()` 立即冲刷待定更新；headless 渲染（`toImage`）前须 `flush()` 或同步策略。
- **通用属性绑定**：`b:` 前缀绑定语义适用于内容类字面量属性（如 `b:color`、`b:background-color`）；布局策略/结构性属性不参与绑定（解析期 BI-P-023 拒绝，不注册）。统一走"字面量 | 绑定"二分解析；解析结果快照在 `ResolvedStyle`（`include/broaditem/core/ResolvedStyle.h`）。

### 顶层结构

`BroadItem`（`include/broaditem/core/BroadItem.h`）继承 `QGraphicsObject`，构造函数中开启 `ItemSendsGeometryChanges|ItemSendsScenePositionChanges`，使 Qt 内部 `Q_PROPERTY` 的 NOTIFY 信号（`xChanged()`、`yChanged()`、`scaleChanged()`、`rotationChanged()`、`opacityChanged()`、`visibleChanged()`）可发射。内部持有 `std::unique_ptr<Frame>`；`Frame` 是布局引擎门面，聚合模板树、实例节点树、属性上下文与布局逻辑，可脱离 QGraphicsItem 使用（见 `frame_widget`/`frame_image` 示例）。

### 模块划分（`include/broaditem/` 与 `src/` 镜像对应）

- `core/`：`BroadItem`、`Frame`、`LayoutEngine`、`Node`、`ResolvedStyle`
- `context/`：属性上下文体系——`PropertyContext`（接口）、`MapPropertyContext`（默认，map 存储）、`QPropertyContext`（proxy 模式，连接目标对象全部 Q_PROPERTY NOTIFY 信号）、`ItemPropertyContext`、`LayoutContext`
- `diagnostics/`：结构化诊断——`Diagnostic`（错误码枚举 BI-P-xxx/BI-R-xxx，级别与恢复策略由码表唯一决定）、`ErrorCollector`（可注入收集器，默认转发 qWarning/qCritical）、`Diagnostics`（`reportParse`/`reportRuntime` 入口；`ParseSession` 解析会话提供元素路径与 Abort 标记，`RuntimeScope` 由 Frame 管线安装、承担模板级去重）。错误码总表见 `doc/设计.md`「诊断」节
- `compat/`：Qt5/Qt6 兼容层（P1-3）。版本差异点唯一落点：`variantIsNull()`（null 语义统一）、`domSetContent()`（setContent 新旧重载）。新增版本差异一律收进本模块，源码中禁止散落的 `QT_VERSION_CHECK`（测试断言除外）
- `expression/`：`Expression`（路径字符串解析）、`Binding`
- `element/`：元素基类层级——`Element` → `ControlElement` / `RenderableElement` → `SizedElement` / `ContainerElement`；`BoxModel`
  - `element/layout/`：`MultiChildContainer`、`ColumnLayout`、`RowLayout`、`GridLayout`、`CellElement`
  - `element/control/`：`ForElement`、`IfElement`（`<if>`：裸 `b:prop` 存在性 / `equals`|`b:equals` 字符串化值比较 / `not` 整体取反不可绑定）
  - `element/text/`：`TextElement`
  - `element/image/`：`ImageElement`（自计算部件；`src` 支持 `:/` Qt 资源路径与文件系统路径；模板级 `QPixmap` 缓存，同一模板多次物化共享）
  - `element/rect/`：`RectElement`（自计算部件；作者定尺寸纯色块，零自有属性；`fillsCrossAxis()` 覆写 true——未给交叉轴尺寸时恒填充，统一覆盖分割线/方形色块/圆形指示灯）
- `parser/`：`XmlLayoutParser`、`LayoutRegistry`
- `reactive/`：QGraphicsItem 实例间纯元对象驱动的属性同步。`ReactiveBinding`（静态 `create()` 工厂，经 `QMetaProperty::notifySignal()` 自动发现 NOTIFY 信号，`QObject::property()`/`setProperty()` 读写，`pos` 走 `xChanged()`/`yChanged()` 特判；支持可选 transform、环检测、源/目标销毁自动清理）、`FollowBinding`、`ConnectionLine`、`AnchorPoint`、`AnchorDecorator`、`ReactiveProperty`（`Property` 属性键常量）。**本版本未与 XML 布局或 `PropertyContext` 集成。**

⚠️ **共存注意**：`QPropertyContext`（proxy 模式）连接目标对象的所有 Q_PROPERTY NOTIFY 信号（`setupNotifyConnections()`），与 `ReactiveBinding` 的元对象连接重叠。当前默认隔离（`BroadItem` 使用 `MapPropertyContext`），但若显式混用需注意 `visible`/`opacity` 等可叠加属性的级联反馈环风险。

## 代码规范

- C++17，命名空间 `BroadItem`。
- 使用 Qt 类型（`QString`、`QVariantMap`、`QPainter`、`QDomElement`）。
- `ElementPtr` = `std::shared_ptr<Element>`。
- 头文件注释用 Doxygen 风格，源文件注释用中文。
- 仓库根目录有 `.clang-tidy`（由 CLion Inspection 设置生成），启用 bugprone-*/cert-*/cppcoreguidelines-部分/modernize-*/performance-*/readability-* 等检查，无 NOLINT 豁免文化的特殊约定；改动可对照该配置自查。
- git commit message 使用中文，格式为 `type(scope): 描述`（如 `feat(binding): …`、`test(golden): …`、`fix(text): …`），scope 取模块名。

## 目录杂项

- `.omo/`、`.zed/`、`.cache/` 为本地工具目录，已被 `.gitignore` 忽略，不属于项目内容。
- 根目录 `.DS_Store`、`.idea/` 等同理勿动。

## 参考文档（均为中文）

- `doc/设计.md`：完整布局系统规格（盒模型、部件、布局算法、模板语言），架构以此为准。
- `doc/设计调整计划.md`：生态位定位与 P0/P1 演进工作清单（P0-1 通用绑定、P0-2 `<image>` 已完成）。
- `doc/主轴等距拉伸.md`：主轴拉伸算法专项设计。
- `doc/属性审阅.md`：属性体系审阅问题清单（逐条核实记录与待决策项）。
