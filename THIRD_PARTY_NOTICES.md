# THIRD_PARTY_NOTICES

本文件列出 BroadItem 以 vendoring 方式（源码内嵌，非外部依赖）使用的第三方组件。
vendoring 协议见 AGENTS.md「依赖与 vendoring」节。

## tl::optional

- **路径**：`third_party/tl/optional.hpp`
- **上游**：https://github.com/TartanLlama/optional
- **版本**：v1.1.0（commit `3a1209de8370bf5fe16362934956144b49591565`）
- **许可证**：CC0 1.0 Universal（公有领域贡献，全文见下）
- **用途**：`std::optional`（C++17）的 C++11 polyfill，经
  `include/broaditem/compat/Optional.h` 以 `BroadItem::Optional` 别名暴露。
- **本地改造**：命名空间 `tl` → `BroadItem::detail::tl`；宏与 include guard
  加 `BI_` 前缀（`TL_` → `BI_TL_`）。改造由 `third_party/revendor.sh` 机械重放。
- **更新策略**：不追新，仅在修 bug 时有意识更新；更新时整文件替换并重放上述改造。

## mpark/variant

- **路径**：`third_party/mpark/`（`variant.hpp` + 内部依赖头 `config.hpp`/`in_place.hpp`/`lib.hpp`）
- **上游**：https://github.com/mpark/variant
- **版本**：v1.4.0（commit `4988879a9f5a95d72308eca2b1779db6ed9b135d`）
- **许可证**：Boost Software License 1.0（全文：https://www.boost.org/LICENSE_1_0.txt）
- **用途**：`std::variant`（C++17）的 C++11 polyfill，经
  `include/broaditem/compat/Variant.h` 以 `BroadItem::Variant<Ts...>` 别名暴露。
- **本地改造**：命名空间 `mpark` → `BroadItem::detail::mpark`；宏与 include guard
  加 `BI_` 前缀（`MPARK_` → `BI_MPARK_`）。改造由 `third_party/revendor.sh` 机械重放。
- **更新策略**：不追新，仅在修 bug 时有意识更新；更新时整目录 4 个头一起替换并重放上述改造。

### CC0 1.0 Universal（声明原文）

The person who associated a work with this deed has dedicated the work to the
public domain by waiving all of his or her rights to the work worldwide under
copyright law, including all related and neighboring rights, to the extent
allowed by law.

You can copy, modify, distribute and perform the work, even for commercial
purposes, all without asking permission.

完整法律文本：http://creativecommons.org/publicdomain/zero/1.0/legalcode
