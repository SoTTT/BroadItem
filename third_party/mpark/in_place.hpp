// ============================================================
// BroadItem vendored copy（由 third_party/revendor.sh 生成，勿手工修改）：
//   上游: https://github.com/mpark/variant v1.4.0 (4988879)
//   本地隔离改造：命名空间 mpark -> BroadItem::detail::mpark；
//                 宏与 include guard 加 BI_ 前缀（MPARK_ -> BI_MPARK_）。
//   更新时整目录（4 个头）一起重跑 revendor.sh。
// ============================================================
// MPark.Variant
//
// Copyright Michael Park, 2015-2017
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)

#ifndef BI_MPARK_IN_PLACE_HPP
#define BI_MPARK_IN_PLACE_HPP

#include <cstddef>

#include "config.hpp"

namespace BroadItem { namespace detail { namespace mpark {

  struct in_place_t { explicit in_place_t() = default; };

  template <std::size_t I>
  struct in_place_index_t { explicit in_place_index_t() = default; };

  template <typename T>
  struct in_place_type_t { explicit in_place_type_t() = default; };

#ifdef BI_MPARK_VARIABLE_TEMPLATES
  constexpr in_place_t in_place{};

  template <std::size_t I> constexpr in_place_index_t<I> in_place_index{};

  template <typename T> constexpr in_place_type_t<T> in_place_type{};
#endif

} } }  // namespace BroadItem::detail::mpark

#endif  // BI_MPARK_IN_PLACE_HPP
