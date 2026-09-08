//
// Eigen 兼容层: X11/X.h (Linux GLFW 传递包含) 定义了
//   #define Success 0
// 会破坏 Eigen 的 ComputationInfo::Success 枚举引用
// (SVDBase.h: m_info(Success) → m_info(0), GCC 报 invalid conversion)。
// 本头在包含 Eigen 前取消该宏; Windows 构建没有 X11, 不受影响。
//
#ifndef FIELD_EIGEN_COMPAT_H
#define FIELD_EIGEN_COMPAT_H

#ifdef Success
#undef Success
#endif

#include <Eigen/Dense>

#endif // FIELD_EIGEN_COMPAT_H
