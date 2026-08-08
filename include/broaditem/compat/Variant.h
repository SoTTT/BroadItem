#pragma once

#include <mpark/variant.hpp>

namespace BroadItem {

/// @brief 和类型，std::variant（C++17）的 polyfill。
///
/// 项目源码按 C++11 基线编写，无法使用 std::variant；
/// 本体为 third_party/mpark/variant.hpp（vendored mpark/variant v1.4.0，
/// 命名空间已隔离为 BroadItem::detail::mpark，见 THIRD_PARTY_NOTICES），
/// 接口与 std::variant 一致（index/get/holds_alternative/visit 等）。
/// 升级 C++17 时将本别名重定向到 std::variant 即可整体退役。
///
/// 使用约定：表达"若干类型之一"的和类型语义（如 PathSegment 的 键|索引）
/// 一律用 Variant，禁止用积类型（全字段 struct + 判别标志）冒充和类型——
/// 积类型会引入"所有字段同时有效"的非法组合状态。
template <typename... Ts>
using Variant = detail::mpark::variant<Ts...>;

} // namespace BroadItem
