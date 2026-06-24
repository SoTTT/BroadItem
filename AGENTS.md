> **写 git commit 和注释必须使用中文。**
> **注释统一使用 Doxygen 风格。**
> **#include 统一使用 <broaditem/module/Header.h> 风格。**

# AGENTS.md — BroadItem

Qt5 C++ library that renders XML-defined layouts as `QGraphicsItem`. Data binding, conditionals, loops.

## Build

```bash
cmake -B build -S . && cmake --build build
```

- Requires Qt5 (Core, Widgets, Xml). `Qt5_DIR` must be discoverable by CMake.
- Library target is `BroadItem` (static). Test/example executables build under `example/` and `tests/`.
- `test_sized_element` was added later and requires `Qt5::Test` (same as other tests).

## Test

```bash
ctest --test-dir build                    # all 8 test suites
./build/tests/test_parser                 # run single test suite
./build/tests/test_property_context       # run single test suite
./build/tests/test_sized_element          # run single test suite
./build/tests/test_layout_behavior        # run single test suite
./build/tests/test_for_element            # run single test suite
./build/tests/test_flatten_children       # run single test suite
./build/tests/test_binding                # run single test suite
./build/tests/test_expression             # run single test suite
./build/example/basic
./build/example/complex
./build/example/playground                # GUI playground for manual experiments
```

- XML layout files used by examples/tests are copied into the build dir via `configure_file` in `example/CMakeLists.txt`. If tests fail to find XMLs, rebuild.
- Test names use the `Test*` naming convention. Each file defines a `QTEST_MAIN(Test*)` and includes its own `.moc`.

## Architecture Notes

- **Three-phase pipeline**: Measure (bottom-up) → Layout (top-down) → Render.
- **Control elements are transparent**: `<for>` and `<if-has>` do **not** participate in measure/layout/render. `ColumnLayout`/`RowLayout`/`GridLayout` call `flattenChildren()` at measure/layout time, which expands control elements into cloned/interpolated ordinary elements via their `expand()` methods.
- **All concrete elements must implement `clone()`** — required for control-element flattening to work.
- **Box model**: margin → border → background → padding → content. Container size = content + decorators. Only content area expands when stretched.
- Data context is a `QVariantMap` (`LayoutContext`). `BroadItem::setDynamicProperty()` triggers relayout only if the bound property is actually used.
- **Reactive module** (`include/broaditem/reactive/`, `src/reactive/`): Item-level property synchronization between `QGraphicsItem` instances via pure meta-object driven reactive bindings. `ObservableGraphicsObject` (base class for `BroadItem`) enables Qt's internal `Q_PROPERTY` NOTIFY signals by setting `ItemSendsGeometryChanges|ItemSendsScenePositionChanges` flags. `ReactiveBinding` (static `create()` factory) accepts `QObject*` source/target, auto-discovers NOTIFY signals via `QMetaProperty::notifySignal()`, reads/writes via `QObject::property()`/`setProperty()`, and handles `pos` specially via `xChanged()`/`yChanged()` connections. Supports optional transform, cycle detection, and automatic cleanup on source/target destruction. `Property` struct provides property key constants. Comments: Doxygen in headers, Chinese in source files. Not integrated with XML layout or `PropertyContext` in this version.
- ⚠️ 共存注意：QPropertyContext（proxy 模式）连接目标对象的所有 Q_PROPERTY NOTIFY 信号（`setupNotifyConnections()`），与 ReactiveBinding 的元对象连接重叠。当前默认隔离（BroadItem 使用 MapPropertyContext），但若显式混用需注意 `visible`/`opacity` 等可叠加属性的级联反馈环风险。

## Code Conventions

- C++17. Namespace `BroadItem`.
- Uses Qt types (`QString`, `QVariantMap`, `QPainter`, `QDomElement`).
- `ElementPtr` = `std::shared_ptr<Element>`.

## Reference

- Full layout specification (Chinese): `doc/设计.md`
