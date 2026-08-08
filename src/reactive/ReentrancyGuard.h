#pragma once

namespace BroadItem {

/// @brief RAII 重入守卫：构造时将布尔标志置位，析构时复位。
///
/// 替代手写的"入口置 true / 出口置 false"重入保护簿记，
/// 保证早退路径下标志一定复位。
/// 重入判定语义因场景而异（静默跳过 / 判定循环并禁用 / 断言），
/// 由调用点在构造守卫前自行检查标志。
/// 本头为 src/reactive 内部实现细节，不对外导出。
class ReentrancyGuard
{
public:
    /// @brief 构造守卫并将标志置位。
    /// @param flag 受保护的布尔标志，生命周期须长于守卫。
    explicit ReentrancyGuard(bool& flag)
        : m_flag(flag)
    {
        m_flag = true;
    }

    /// @brief 析构时复位标志。
    ~ReentrancyGuard()
    {
        m_flag = false;
    }

    ReentrancyGuard(const ReentrancyGuard&) = delete;
    ReentrancyGuard& operator=(const ReentrancyGuard&) = delete;

private:
    bool& m_flag;
};

} // namespace BroadItem
