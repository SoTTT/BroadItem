#pragma once

#include <tl/optional.hpp>

namespace BroadItem {

/// @brief 可空值类型，std::optional（C++17）的 polyfill。
///
/// 项目源码按 C++11 基线编写，无法使用 std::optional；
/// 本体为 third_party/tl/optional.hpp（vendored tl::optional v1.1.0，
/// 命名空间已隔离为 BroadItem::detail::tl，见 THIRD_PARTY_NOTICES），
/// 接口与 std::optional 一致（has_value/value/value_or/operator* 等）。
/// 升级 C++17 时将本别名重定向到 std::optional 即可整体退役。
///
/// 使用约定：值域有天然非法值时用哨兵值（如 baselineOffset 的 -1），
/// 其余"可空"语义一律用 Optional，禁止散落的指针/魔数表达。
template <typename T>
using Optional = detail::tl::optional<T>;

/// @brief nullopt 标签类型（对齐 std::nullopt_t）。
using NulloptT = detail::tl::nullopt_t;

/// @brief 空值常量（对齐 std::nullopt），用于显式表达"无值"与默认参数。
constexpr NulloptT nullopt{detail::tl::nullopt_t::do_not_use{}, detail::tl::nullopt_t::do_not_use{}};

} // namespace BroadItem
