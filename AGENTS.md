> **写 git commit 和注释必须使用中文。**
> **注释统一使用 Doxygen 风格。**
> **#include 统一使用 <broaditem/module/Header.h> 风格。**

# AGENTS.md — BroadItem

Qt5/Qt6 双栈 C++ 静态库：从 XML 文件加载布局，渲染为 `QGraphicsItem`，支持数据绑定、条件分支（`<if>`）与循环（`<for>`）。

定位：给已被锁定在 QGraphicsView 技术栈里的既有应用提供数据驱动的场景内信息标牌（对标 Qwt 的生存策略——零依赖、静态库、可整体 vendor）。定位结论与演进计划见 `doc/设计调整计划.md`。

## 构建

```bash
cmake -B build -S . && cmake --build build          # Qt5（CMAKE_PREFIX_PATH 指向 qt@5）
cmake -B build-qt6 -S . -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt@6 && cmake --build build-qt6   # Qt6
```

- **Qt5/Qt6 双栈**：`find_package(QT NAMES Qt6 Qt5)` 优先 Qt6、回退 Qt5，链接目标统一 `Qt${QT_VERSION_MAJOR}::`；依赖 Core、Widgets、Xml（测试还需 Test）。版本差异点集中在 `compat/` 模块（P1-3 纪律：禁止散落版本分支），目前有 `variantIsNull()`（Qt6 的 `QVariant::isNull()` 不再传播内含类型的 null 性）与 `domSetContent()`（Qt6.8 起 setContent 旧重载废弃）。
- **已知行为差异**：Qt6 的 XML 解析器更严格——未声明命名空间前缀（`b:content` 无 `xmlns:b`、裸 `:content`）在 Qt5 下"parse 成功但无绑定"，Qt6 下为 BI-P-002 语法错误 parse 失败；两版语义等价（均不产生绑定），测试按 `QT_VERSION_CHECK` 分别断言。
- **C++11 基线**：源码按 C++11 编写（生态位面向 Qt5 时代的老工程，可直接 vendor）；Qt6 头文件硬性要求 C++17，Qt6 构建时 `CMAKE_CXX_STANDARD` 自动提到 17。高版本标准库特性由 polyfill 补齐：C++14 的 `std::make_unique` 用 `compat/` 模块的 `makeUnique()`，C++17 的 `std::optional` 用 `compat/Optional.h` 的 `Optional<T>`（vendored tl::optional，见「依赖与 vendoring」）。开启 AUTOMOC/AUTORCC/AUTOUIC，导出 `compile_commands.json`。已知例外：存量公共头中的 `[[nodiscard]]`（reactive 模块为主）是 C++17 属性，两栈编译器按扩展静默接受，暂保留待清理。
- **安装与下游消费**：`cmake --install <build-dir> --prefix <prefix>` 安装头文件、静态库、`broaditem.xsd`、LICENSE 与 CMake 包配置；导出目标 `BroadItem::BroadItem`（`add_subdirectory` 消费有同名 ALIAS）。`BroadItemConfig.cmake` 烧入构建时的 Qt 主版本并自动 `find_dependency`，下游须用同一 Qt 主版本；`cxx_std_11` 以 PUBLIC compile feature 随目标传导（Qt6 构建由 Qt6 接口特性自动提至 17）。版本 0.x 阶段按 `SameMinorVersion` 判定兼容，发布以 git tag `vX.Y.Z` 为准。
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
- `test_diagnostics`：P1-1 结构化诊断。码表 32 个错误码（BI-P-001~025 有跳号 + BI-R-002~011 有跳号）逐一一个用例 + 行为用例（嵌套 Abort 整文件失败、全收集、运行时实例级去重、并发 parse 隔离、静默清单、Default 语义回归）
- `test_image_element`：`<image>` 部件测试，首个 qrc 测试基建（`tests/assets/icons.qrc` 经 `qt5/qt6_add_resources` 编入该目标）
- `test_rect_element`：`<rect>` 部件测试（逐维度 measure、fillsCrossAxis 交叉轴恒填充、显式尺寸豁免与居中、绑定求值与拒绝路径、cell/根退化、BI-P-007）
- `test_golden_render`：黄金镜像校验。`tests/golden/golden_render.cpp` 是采集/校验工具（`--capture <dir>` 按内置 manifest 渲染 PNG；`--verify <dir>` 逐像素比对，等价组不一致则退出码 1）。**该测试固定 `QT_QPA_PLATFORM=offscreen`**（`set_tests_properties`），cocoa 下字体光栅化不确定性会破坏逐像素比对，改金图基建时不得去掉此环境变量。**基线按 Qt 版本分套**：`tests/golden/golden/`（Qt5）与 `tests/golden/golden-qt6/`（Qt6，字体度量/光栅化有亚像素级漂移，逐像素比对不可跨栈共用），`tests/CMakeLists.txt` 按 `QT_VERSION_MAJOR` 指向对应基线；改渲染行为时两套都要重新采集。

注意：

- 普通测试目标经 `tests/CMakeLists.txt` 的 `bi_add_plain_test(<name>)` 函数注册（`BI_PLAIN_TESTS` 列表 + foreach），新增测试只需在列表加一行；qrc、金图等特例在该函数之外单独处理。
- 跨套件共享的测试辅助收敛在 `tests/helpers/`（`binding_helpers.h`/`layout_helpers.h`/`diagnostics_helpers.h`/`reactive_helpers.h`/`frame_helpers.h`，`BroadItem::TestHelpers` 命名空间），新增重复 helper 时优先放这里。注意：`reactive_helpers.h` 含 Q_OBJECT 类，AUTOMOC 不会自动 moc 子目录头文件，消费该头的 cpp 须在末尾 `#include "helpers/moc_reactive_helpers.cpp"`；新增含 Q_OBJECT 的共享头沿用此模式。
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
./build/example/path_tester       # 路径语法实验工具
```

`example/badges/` 布局集由 `badges` 可执行集中展示；单张 PNG 用 `./build/example/frame_image <布局.xml> <输出.png>` 渲染（存于 `doc/images/badges/`）。`frame_image` 第三参数可指定布局可用宽度，缺省 -1 为内容驱动。示例间共享的离屏渲染/`--smoke`/`--shot` 参数处理收敛在 `example/common/scene_shot.h`。

## 架构

四阶段流水线：**Materialize**（模板层 → 实例层 `Node` 树）→ **Measure**（自底向上）→ **Layout**（自顶向下）→ **Render**。

- **模板/实例分离**：模板层 `Element` 树在 `parse()` 后不可变，可安全共享；不可变性由类型强制——共享边界（`LayoutRegistry`、`Frame`、`LayoutEngine`）一律 `ConstElementPtr`（`shared_ptr<const Element>`），仅解析期构建用 `ElementPtr`（doc/模板只读边界.md）；每实例状态（布局矩形、物化子节点）存于实例层 `Node` 树。
- **控制元素透明**：`<for>` 和 `<if>` 不产生 `Node`，不参与 measure/layout/render。`ColumnLayout`/`RowLayout`/`GridLayout` 在物化时调用 `materializeChildren()`，把控制元素结构性地展开为 0..N 个普通元素实例节点。
- **盒模型**：margin → border → background → padding → content。容器尺寸 = content + 装饰层；拉伸时只有 content 区域扩展。
- **数据上下文**：`LayoutContext` 基于 `QVariantMap`。`BroadItem::setDynamicProperty()` 仅当被绑定属性确实被使用时才触发重布局。变更合并：命中后标脏并按 `UpdatePolicy` 调度——默认 `Coalesced`（`QTimer::singleShot(0)` 合并同一事件循环回合内的多次变更为一次重布局），`Synchronous` 可选；`Frame`/`BroadItem` 均有 `flush()` 立即冲刷待定更新；headless 渲染（`toImage`）前须 `flush()` 或同步策略。
- **通用属性绑定**：`b:` 前缀绑定语义适用于内容类字面量属性（如 `b:color`、`b:background-color`）；布局策略/结构性属性不参与绑定（解析期 BI-P-023 拒绝，不注册）。统一走"字面量 | 绑定"二分解析；解析结果快照在 `ResolvedStyle`（`include/broaditem/core/ResolvedStyle.h`）。

### 顶层结构

`BroadItem`（`include/broaditem/core/BroadItem.h`）继承 `QGraphicsObject`，构造函数中开启 `ItemSendsGeometryChanges|ItemSendsScenePositionChanges`，使 Qt 内部 `Q_PROPERTY` 的 NOTIFY 信号（`xChanged()`、`yChanged()`、`scaleChanged()`、`rotationChanged()`、`opacityChanged()`、`visibleChanged()`）可发射。内部持有 `std::unique_ptr<Frame>`；`Frame` 是布局引擎门面，聚合模板树、实例节点树、属性上下文与布局逻辑，可脱离 QGraphicsItem 使用（见 `frame_widget`/`frame_image` 示例）。

### 模块划分（`include/broaditem/` 与 `src/` 镜像对应）

- `core/`：`BroadItem`、`Frame`、`LayoutEngine`（管线唯一入口，`Frame::performLayout`/`paint` 委托其静态方法，经 `Node::element` 派发）、`Node`、`ResolvedStyle`
- `context/`：属性上下文体系——`PropertyContext`（接口）、`MapPropertyContext`（默认，map 存储）、`LayoutContext`；`<for>` 迭代作用域经 `LayoutContext::Scope` 栈链表达（项级优先、as 前缀剥离、回退全局），无独立上下文类
- `diagnostics/`：结构化诊断——`Diagnostic`（错误码枚举 BI-P-xxx/BI-R-xxx，级别与恢复策略由码表唯一决定）、`ErrorCollector`（可注入收集器，默认转发 qWarning/qCritical）、`Diagnostics`（`reportParse`/`reportRuntime` 入口；`ParseSession` 解析会话提供元素路径与 Abort 标记，`RuntimeScope` 由 Frame 管线安装、承担实例级去重，状态为 Frame 成员）。线程模型：作用域栈 thread_local，进程级收集器读写经互斥锁（锁内拷贝、锁外调用）。错误码总表见 `doc/设计.md`「诊断」节
- `compat/`：Qt5/Qt6 兼容层（P1-3）+ C++ 标准库 polyfill。版本差异点唯一落点：`variantIsNull()`（null 语义统一）、`domSetContent()`（setContent 新旧重载）、`makeUnique()`（C++14 `make_unique`）、`Optional<T>`（C++17 `optional`，`Optional.h` 内为 vendored tl::optional 的别名，接口对齐 std 以便升标后整体退役）。新增版本差异一律收进本模块，源码中禁止散落的 `QT_VERSION_CHECK`（测试断言除外）
- `expression/`：`Expression`（路径语法唯一权威：字符串 → 类型化分段 `PathSegment`，运行期遍历一律消费分段）、`Binding`
- `element/`：元素基类层级——`Element` → `ControlElement` / `RenderableElement` → `SizedElement` / `ContainerElement`；`BoxModel`。解析期挂载协议：parser 统一经虚函数 `addParsedChild()`（挂载子元素）与 `validateChildren()`（结构校验）驱动，新增部件自带挂载/校验语义，parser 零改动（doc/解析器挂载下沉.md）
  - `element/layout/`：`MultiChildContainer`、`ColumnLayout`、`RowLayout`、`GridLayout`、`CellElement`；Row/Column 的轴无关公共实现（parse、main-align 分发布、main-stretch 双遍测量）收口在 src 内部单元 `src/element/layout/LinearLayoutCommon.h/.cpp`（不进公共 include 树）
  - `element/control/`：`ForElement`、`IfElement`（`<if>`：裸 `b:prop` 存在性 / `equals`|`b:equals` 字符串化值比较 / `not` 整体取反不可绑定）
  - `element/text/`：`TextElement`
  - `element/image/`：`ImageElement`（自计算部件；`src` 支持 `:/` Qt 资源路径与文件系统路径；图像缓存为进程级路径键缓存 `src/element/image/ImageCache`，含互斥锁、跨模板共享）
  - `element/rect/`：`RectElement`（自计算部件；作者定尺寸纯色块，零自有属性；`fillsCrossAxis()` 覆写 true——未给交叉轴尺寸时恒填充，统一覆盖分割线/方形色块/圆形指示灯）
- `parser/`：`XmlLayoutParser`、`LayoutRegistry`
- `reactive/`：QGraphicsItem 实例间纯元对象驱动的属性同步。`ReactiveBinding`（静态 `create()` 工厂，经 `QMetaProperty::notifySignal()` 自动发现 NOTIFY 信号，`QObject::property()`/`setProperty()` 读写，`pos` 走 `xChanged()`/`yChanged()` 特判；支持可选 transform、环检测、源/目标销毁自动清理）、`FollowBinding`、`ConnectionLine`、`AnchorPoint`、`AnchorDecorator`、`ReactiveProperty`（`Property` 属性键常量）。z 值层级约定（连接线与装饰器 1、锚点 2）收口在 src 内部头 `src/reactive/ZOrder.h`。**本版本未与 XML 布局或 `PropertyContext` 集成。**

## 代码规范

- C++11，命名空间 `BroadItem`。
- 使用 Qt 类型（`QString`、`QVariantMap`、`QPainter`、`QDomElement`）。
- `ElementPtr` = `std::shared_ptr<Element>`。
- 头文件注释用 Doxygen 风格，源文件注释用中文。
- 仓库根目录有 `.clang-tidy`（由 CLion Inspection 设置生成），启用 bugprone-*/cert-*/cppcoreguidelines-部分/modernize-*/performance-*/readability-* 等检查，无 NOLINT 豁免文化的特殊约定；改动可对照该配置自查。
- git commit message 使用中文，格式为 `type(scope): 描述`（如 `feat(binding): …`、`test(golden): …`、`fix(text): …`），scope 取模块名。

## 依赖与 vendoring

- **禁止需要下游感知的依赖**：不允许新增编译期/链接期/包管理期外部依赖（boost、absl 级别一律免谈）。
- **允许单头 vendoring**：功能单一、许可证宽松的单头文件库可以源码内嵌，协议如下：
  1. 统一放 `third_party/`，与业务源码物理隔离；
  2. 命名空间隔离为**每库独立子命名空间** `BroadItem::detail::<lib>`（防库间同名符号冲突——tl 与 mpark 均有 monostate/in_place_t，共用一层必撞），宏/include guard 加 `BI_` 前缀；改造由 `third_party/revendor.sh` 机械重放，禁止手改产物；
  3. `THIRD_PARTY_NOTICES.md` 登记名称、上游 URL、版本/commit、许可证与本地改造；
  4. 不追新，仅在修 bug 时有意识更新，更新时整文件替换并重放隔离改造；
  5. 接口须镜像对应 std 组件（一次性用品，升标准后整体退役）。
- **绊线**：vendored 组件或自制 polyfill 合计超过 5 个，说明判断有误，重新评审依赖策略而不是继续堆。
- 当前 vendored 清单：`third_party/tl/optional.hpp`（tl::optional v1.1.0，CC0）、`third_party/mpark/`（mpark/variant v1.4.0，Boost 1.0，4 个头）。
- 可空语义表达约定：值域有天然非法值时用哨兵值，其余一律 `Optional<T>`（`compat/Optional.h`）；和类型语义（"若干类型之一"）一律 `Variant<Ts...>`（`compat/Variant.h`）。禁止散落的指针/魔数表达，禁止积类型冒充和类型。

## 目录杂项

- `.omo/`、`.zed/`、`.cache/` 为本地工具目录，已被 `.gitignore` 忽略，不属于项目内容。
- 根目录 `.DS_Store`、`.idea/` 等同理勿动。

## 参考文档（均为中文）

- `doc/设计.md`：完整布局系统规格（盒模型、部件、布局算法、模板语言），架构以此为准。
- `doc/设计调整计划.md`：生态位定位与 P0/P1 演进工作清单（P0-1 通用绑定、P0-2 `<image>` 已完成）。
- `doc/主轴等距拉伸.md`：主轴拉伸算法专项设计。
- `doc/属性审阅.md`：属性体系审阅问题清单（逐条核实记录，已全结，留作审阅过程档案）。
- `doc/土法编程审阅.md`：土法编程（hand-rolled 实现）问题清单，已全结，留作审阅过程档案。
- `doc/路径语法.md`：绑定路径语法规范与单一权威设计（`Expression` + 类型化分段 IR）。
- `doc/解析器挂载下沉.md`：解析期挂载协议设计（`addParsedChild`/`validateChildren` 虚函数 + 工厂查表）。
- `doc/模板只读边界.md`：模板不可变性的类型强制（`ConstElementPtr` 共享边界）。
- `doc/运行期诊断状态设计草案.md`：运行期诊断状态的设计草案。
