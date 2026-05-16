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
ctest --test-dir build                    # test_parser + test_property_context + test_sized_element
./build/tests/test_parser                 # run single test suite
./build/tests/test_property_context       # run single test suite
./build/tests/test_sized_element          # run single test suite
./build/example/test_basic
./build/example/test_complex
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

## Code Conventions

- C++17. Namespace `BroadItem`.
- Uses Qt types (`QString`, `QVariantMap`, `QPainter`, `QDomElement`).
- `ElementPtr` = `std::shared_ptr<Element>`.

## Reference

- Full layout specification (Chinese): `doc/设计.md`
