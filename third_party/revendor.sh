#!/bin/bash
# BroadItem vendored 依赖的获取与隔离改造重放脚本（vendoring 协议见 AGENTS.md）。
# 用途：更新 vendored 组件版本时，修改下方钉住的 commit 后整目录重跑本脚本。
# 隔离规则：命名空间 <lib> -> BroadItem::detail::<lib>；宏与 include guard 加 BI_ 前缀。
set -euo pipefail
cd "$(dirname "$0")"

TL_OPTIONAL_COMMIT=3a1209de8370bf5fe16362934956144b49591565   # v1.1.0
MPARK_VARIANT_COMMIT=4988879a9f5a95d72308eca2b1779db6ed9b135d  # v1.4.0

CDN="https://cdn.jsdelivr.net/gh"

# ---------- tl::optional ----------
mkdir -p tl
curl -sL --fail -o tl/optional.hpp \
    "$CDN/TartanLlama/optional@$TL_OPTIONAL_COMMIT/include/tl/optional.hpp"

perl -0pi -e '
s/\bTL_/BI_TL_/g;
s/^namespace tl \{$/namespace BroadItem { namespace detail { namespace tl {/gm;
s/^  \}\n\}\n#endif$/  }\n} } }\n#endif/m;
s/^\} \/\/ namespace tl$/} } } \/\/ namespace BroadItem::detail::tl/gm;
s/\btl::/BroadItem::detail::tl::/g;
' tl/optional.hpp

cat > /tmp/bi_banner_tl.txt <<'EOF'
// ============================================================
// BroadItem vendored copy（由 third_party/revendor.sh 生成，勿手工修改）：
//   上游: https://github.com/TartanLlama/optional v1.1.0 (3a1209d)
//   本地隔离改造：命名空间 tl -> BroadItem::detail::tl；
//                 宏与 include guard 加 BI_ 前缀（TL_ -> BI_TL_）。
//   注：两处 "}; // namespace tl" 为上游注释笔误，保持原样。
// ============================================================
EOF
cat /tmp/bi_banner_tl.txt tl/optional.hpp > tl/optional.hpp.tmp
mv tl/optional.hpp.tmp tl/optional.hpp

# ---------- mpark/variant（variant.hpp + 3 个内部依赖头）----------
mkdir -p mpark
for f in variant.hpp config.hpp in_place.hpp lib.hpp; do
    curl -sL --fail -o "mpark/$f" \
        "$CDN/mpark/variant@$MPARK_VARIANT_COMMIT/include/mpark/$f"

    perl -0pi -e '
s/\bMPARK_/BI_MPARK_/g;
s/^namespace mpark \{$/namespace BroadItem { namespace detail { namespace mpark {/gm;
s/^\}  \/\/ namespace mpark$/} } }  \/\/ namespace BroadItem::detail::mpark/gm;
s/\bmpark::/BroadItem::detail::mpark::/g;
' "mpark/$f"

    cat > /tmp/bi_banner_mpark.txt <<'EOF'
// ============================================================
// BroadItem vendored copy（由 third_party/revendor.sh 生成，勿手工修改）：
//   上游: https://github.com/mpark/variant v1.4.0 (4988879)
//   本地隔离改造：命名空间 mpark -> BroadItem::detail::mpark；
//                 宏与 include guard 加 BI_ 前缀（MPARK_ -> BI_MPARK_）。
//   更新时整目录（4 个头）一起重跑 revendor.sh。
// ============================================================
EOF
    cat /tmp/bi_banner_mpark.txt "mpark/$f" > "mpark/$f.tmp"
    mv "mpark/$f.tmp" "mpark/$f"
done

rm -f /tmp/bi_banner_tl.txt /tmp/bi_banner_mpark.txt
echo "revendor 完成：tl/optional.hpp + mpark/{variant,config,in_place,lib}.hpp"
