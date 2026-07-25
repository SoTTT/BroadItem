> **写 git commit 和注释必须使用中文。**
> **注释统一使用 Doxygen 风格。**
> **#include 统一使用 <broaditem/module/Header.h> 风格。**

# AGENTS.md — BroadItem

Qt5 C++ 静态库：从 XML 文件加载布局，渲染为 `QGraphicsItem`，支持数据绑定、条件分支（`<if>`）与循环（`<for>`）。

定位：给已被锁定在 QGraphicsView 技术栈里的既有应用提供数据驱动的场景内信息标牌（对标 Qwt 的生存策略——零依赖、静态库、可整体 vendor）。定位结论与演进计划见 `doc/设计调整计划.md`。

## 构建

```bash
cmake -B build -S . && cmake --build build
```

- 依赖 Qt5（Core、Widgets、Xml；测试还需 Test），`Qt5_DIR` 须可被 CMake 发现。
- C++17，开启 AUTOMOC/AUTORCC/AUTOUIC，导出 `compile_commands.json`。
- 库目标为 `BroadItem`（STATIC）。示例与测试可执行文件分别构建于 `example/` 和 `tests/` 子目录。
- 仓库根目录另有 `cmake-build-debug/`（CLion 的 Ninja 构建目录），`build/` 才是 `.gitignore` 认可的标准构建目录。

## 测试

```bash
ctest --test-dir build --output-on-failure   # 全部 15 个 ctest 条目
./build/tests/test_parser                    # 运行单个测试套件
```

测试套件（均在 `tests/`，`QTEST_MAIN(Test*)` + 包含自身 `.moc`，链接 `Qt5::Test`）：

- `test_parser`、`test_property_context`、`test_sized_element`、`test_layout_behavior`、`test_for_element`、`test_flatten_children`、`test_binding`、`test_expression`、`test_reactive_binding`、`test_connection_line`、`test_anchor_decorator`、`test_regression`、`test_bound_attributes`
- `test_image_element`：`<image>` 部件测试，首个 qrc 测试基建（`tests/assets/icons.qrc` 经 `qt5_add_resources` 编入该目标）
- `test_golden_render`：黄金镜像校验。`tests/golden/golden_render.cpp` 是采集/校验工具（`--capture <dir>` 按内置 manifest 渲染 PNG；`--verify <dir>` 逐像素比对，等价组不一致则退出码 1）。**该测试固定 `QT_QPA_PLATFORM=offscreen`**（`set_tests_properties`），cocoa 下字体光栅化不确定性会破坏逐像素比对，改金图基建时不得去掉此环境变量。

注意：

- 示例/测试使用的 XML 布局文件经 `configure_file` 拷入构建目录（见 `example/CMakeLists.txt`、`tests/CMakeLists.txt`）。测试报找不到 XML 时先重新构建。
- 黄金基线 PNG 位于 `tests/golden/golden/`，对应布局在 `tests/golden/layouts/`。
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
```

## 架构

四阶段流水线：**Materialize**（模板层 → 实例层 `Node` 树）→ **Measure**（自底向上）→ **Layout**（自顶向下）→ **Render**。

- **模板/实例分离**：模板层 `Element` 树在 `parse()` 后不可变，可安全共享（`LayoutRegistry` 按 layoutId 原样返回 `ElementPtr`，支持多实例共用同一布局）；每实例状态（布局矩形、物化子节点）存于实例层 `Node` 树。
- **控制元素透明**：`<for>` 和 `<if>` 不产生 `Node`，不参与 measure/layout/render。`ColumnLayout`/`RowLayout`/`GridLayout` 在物化时调用 `materializeChildren()`，把控制元素结构性地展开为 0..N 个普通元素实例节点。
- **盒模型**：margin → border → background → padding → content。容器尺寸 = content + 装饰层；拉伸时只有 content 区域扩展。
- **数据上下文**：`LayoutContext` 基于 `QVariantMap`。`BroadItem::setDynamicProperty()` 仅当被绑定属性确实被使用时才触发重布局。
- **通用属性绑定**：`b:` 前缀绑定语义适用于全部字面量属性（如 `b:color`、`b:background-color`），统一走"字面量 | 绑定"二分解析；解析结果快照在 `ResolvedStyle`（`include/broaditem/core/ResolvedStyle.h`）。

### 顶层结构

`BroadItem`（`include/broaditem/core/BroadItem.h`）继承 `QGraphicsObject`，构造函数中开启 `ItemSendsGeometryChanges|ItemSendsScenePositionChanges`，使 Qt 内部 `Q_PROPERTY` 的 NOTIFY 信号（`xChanged()`、`yChanged()`、`scaleChanged()`、`rotationChanged()`、`opacityChanged()`、`visibleChanged()`）可发射。内部持有 `std::unique_ptr<Frame>`；`Frame` 是布局引擎门面，聚合模板树、实例节点树、属性上下文与布局逻辑，可脱离 QGraphicsItem 使用（见 `frame_widget`/`frame_image` 示例）。

### 模块划分（`include/broaditem/` 与 `src/` 镜像对应）

- `core/`：`BroadItem`、`Frame`、`LayoutEngine`、`Node`、`ResolvedStyle`
- `context/`：属性上下文体系——`PropertyContext`（接口）、`MapPropertyContext`（默认，map 存储）、`QPropertyContext`（proxy 模式，连接目标对象全部 Q_PROPERTY NOTIFY 信号）、`ItemPropertyContext`、`LayoutContext`
- `expression/`：`Expression`（路径字符串解析）、`Binding`
- `element/`：元素基类层级——`Element` → `ControlElement` / `RenderableElement` → `SizedElement` / `ContainerElement`；`BoxModel`
  - `element/layout/`：`MultiChildContainer`、`ColumnLayout`、`RowLayout`、`GridLayout`、`CellElement`
  - `element/control/`：`ForElement`、`IfElement`（`<if>`：裸 `b:prop` 存在性 / `equals`|`b:equals` 字符串化值比较 / `not` 整体取反不可绑定）
  - `element/text/`：`TextElement`
  - `element/image/`：`ImageElement`（自计算部件；`src` 支持 `:/` Qt 资源路径与文件系统路径；模板级 `QPixmap` 缓存，同一模板多次物化共享）
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
