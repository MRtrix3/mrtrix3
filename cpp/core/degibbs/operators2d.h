/* Copyright (c) 2008-2026 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#pragma once

#include <Eigen/Core>
#include <algorithm>
#include <cmath>

#include "degibbs/degibbs.h"
#include "math/math.h"

namespace MR::Mrdegibbs2 {

using MR::Degibbs::complex_type;
using MR::Degibbs::real_type;
using complex_matrix = Eigen::Matrix<complex_type, Eigen::Dynamic, Eigen::Dynamic>;
using real_matrix = Eigen::Matrix<real_type, Eigen::Dynamic, Eigen::Dynamic>;

//! k-space apodisation window kinds.
enum class WindowKind { Rect, Tukey, Hamming };

/*! Forward gradient with Neumann (zero-extension) boundary conditions:
 *  gx(i,j) = u(i+1,j) - u(i,j) for i < M-1, gx(M-1,:) = 0
 *  gy(i,j) = u(i,j+1) - u(i,j) for j < N-1, gy(:,N-1) = 0
 *  Operator norm bound: \f$ \|\nabla\|^2 \le 8 \f$.
 */
inline void grad_op(const complex_matrix &u, complex_matrix &gx, complex_matrix &gy) {
  const Eigen::Index M = u.rows();
  const Eigen::Index N = u.cols();
  assert(M >= 2 && N >= 2);
  gx.setZero(M, N);
  gy.setZero(M, N);
  gx.topRows(M - 1) = u.bottomRows(M - 1) - u.topRows(M - 1);
  gy.leftCols(N - 1) = u.rightCols(N - 1) - u.leftCols(N - 1);
}

/*! Backward divergence: \f$ \mathrm{div}(p) = -(\nabla)^*(p) \f$.
 *  div_x(p)(i,j) = p_x(i,j) - p_x(i-1,j) with p_x(-1,:) = p_x(M-1,:) = 0;
 *  div_y(p)(i,j) = p_y(i,j) - p_y(i,j-1) with analogous BC.
 *  out = div_x(px) + div_y(py).
 */
inline void div_op(const complex_matrix &px, const complex_matrix &py, complex_matrix &out) {
  const Eigen::Index M = px.rows();
  const Eigen::Index N = px.cols();
  assert(M >= 2 && N >= 2);
  assert(py.rows() == M && py.cols() == N);
  out.setZero(M, N);
  // div_x(px)
  out.row(0) += px.row(0);
  if (M > 2)
    out.middleRows(1, M - 2) += px.middleRows(1, M - 2) - px.topRows(M - 2);
  out.row(M - 1) += -px.row(M - 2);
  // div_y(py)
  out.col(0) += py.col(0);
  if (N > 2)
    out.middleCols(1, N - 2) += py.middleCols(1, N - 2) - py.leftCols(N - 2);
  out.col(N - 1) += -py.col(N - 2);
}

/*! Symmetrised gradient (Jacobian) of a vector field v = (vx, vy):
 *  qxx = ∂_x vx, qyy = ∂_y vy, qxy = (1/2)(∂_y vx + ∂_x vy).
 *  Forward differences with Neumann BC; operator norm \f$ \|\epsilon\|^2 \le 8 \f$.
 */
inline void sym_grad_op(
    const complex_matrix &vx, const complex_matrix &vy, complex_matrix &qxx, complex_matrix &qyy, complex_matrix &qxy) {
  const Eigen::Index M = vx.rows();
  const Eigen::Index N = vx.cols();
  assert(M >= 2 && N >= 2);
  assert(vy.rows() == M && vy.cols() == N);
  qxx.setZero(M, N);
  qyy.setZero(M, N);
  qxy.setZero(M, N);
  // qxx = ∂_x vx
  qxx.topRows(M - 1) = vx.bottomRows(M - 1) - vx.topRows(M - 1);
  // qyy = ∂_y vy
  qyy.leftCols(N - 1) = vy.rightCols(N - 1) - vy.leftCols(N - 1);
  // qxy = (1/2)(∂_y vx + ∂_x vy)
  qxy.leftCols(N - 1) = vx.rightCols(N - 1) - vx.leftCols(N - 1);
  qxy.topRows(M - 1) += vy.bottomRows(M - 1) - vy.topRows(M - 1);
  qxy *= real_type(0.5);
}

/*! Adjoint of -ε, i.e. \f$ \mathrm{sym\_div}(q) = -\epsilon^*(q) \f$.
 *  Using the symmetric tensor inner product \f$ \langle Q, R \rangle =
 *  Q_{xx}R_{xx} + Q_{yy}R_{yy} + 2 Q_{xy}R_{xy} \f$, the adjoint of ε reads
 *  \f$ \epsilon^*(q)_x = -\mathrm{div}_x(q_{xx}) - \mathrm{div}_y(q_{xy}) \f$ and
 *  \f$ \epsilon^*(q)_y = -\mathrm{div}_y(q_{yy}) - \mathrm{div}_x(q_{xy}) \f$;
 *  hence \f$ \mathrm{sym\_div}(q)_x = \mathrm{div}_x(q_{xx}) + \mathrm{div}_y(q_{xy}) \f$, etc.
 */
inline void sym_div_op(const complex_matrix &qxx,
                       const complex_matrix &qyy,
                       const complex_matrix &qxy,
                       complex_matrix &rx,
                       complex_matrix &ry) {
  const Eigen::Index M = qxx.rows();
  const Eigen::Index N = qxx.cols();
  assert(M >= 2 && N >= 2);
  assert(qyy.rows() == M && qyy.cols() == N);
  assert(qxy.rows() == M && qxy.cols() == N);
  rx.setZero(M, N);
  ry.setZero(M, N);
  // div_x(qxx) → rx
  rx.row(0) += qxx.row(0);
  if (M > 2)
    rx.middleRows(1, M - 2) += qxx.middleRows(1, M - 2) - qxx.topRows(M - 2);
  rx.row(M - 1) += -qxx.row(M - 2);
  // div_y(qxy) → rx
  rx.col(0) += qxy.col(0);
  if (N > 2)
    rx.middleCols(1, N - 2) += qxy.middleCols(1, N - 2) - qxy.leftCols(N - 2);
  rx.col(N - 1) += -qxy.col(N - 2);
  // div_y(qyy) → ry
  ry.col(0) += qyy.col(0);
  if (N > 2)
    ry.middleCols(1, N - 2) += qyy.middleCols(1, N - 2) - qyy.leftCols(N - 2);
  ry.col(N - 1) += -qyy.col(N - 2);
  // div_x(qxy) → ry
  ry.row(0) += qxy.row(0);
  if (M > 2)
    ry.middleRows(1, M - 2) += qxy.middleRows(1, M - 2) - qxy.topRows(M - 2);
  ry.row(M - 1) += -qxy.row(M - 2);
}

/*! Pointwise projection of the gradient dual onto the L2 ball of radius α.
 *  The pointwise norm couples the two gradient directions and (implicitly) the
 *  real and imaginary parts of each component:
 *    \f$ \|p[i,j]\|^2 = |p_x[i,j]|^2 + |p_y[i,j]|^2 \f$.
 *  An alternative would be to project real and imaginary parts independently
 *  (decoupled TGV); we follow the coupled (channel-joint) convention of
 *  Knoll et al. 2011 for MRI.
 */
inline void project_grad_dual(complex_matrix &px, complex_matrix &py, real_type alpha) {
  const Eigen::Index M = px.rows();
  const Eigen::Index N = px.cols();
  assert(py.rows() == M && py.cols() == N);
  for (Eigen::Index j = 0; j < N; ++j) {
    for (Eigen::Index i = 0; i < M; ++i) {
      const real_type norm = std::sqrt(std::norm(px(i, j)) + std::norm(py(i, j)));
      if (norm > alpha) {
        const real_type scale = alpha / norm;
        px(i, j) *= scale;
        py(i, j) *= scale;
      }
    }
  }
}

/*! Pointwise projection of the symmetric tensor dual onto the L2 ball of radius α.
 *  The pointwise (Frobenius) norm of a 2×2 symmetric tensor with unique entries
 *  (qxx, qyy, qxy) is \f$ \|q[i,j]\|^2 = |q_{xx}|^2 + |q_{yy}|^2 + 2 |q_{xy}|^2 \f$,
 *  again with real/imaginary parts contributing jointly.
 */
inline void project_sym_dual(complex_matrix &qxx, complex_matrix &qyy, complex_matrix &qxy, real_type alpha) {
  const Eigen::Index M = qxx.rows();
  const Eigen::Index N = qxx.cols();
  assert(qyy.rows() == M && qyy.cols() == N);
  assert(qxy.rows() == M && qxy.cols() == N);
  for (Eigen::Index j = 0; j < N; ++j) {
    for (Eigen::Index i = 0; i < M; ++i) {
      const real_type norm =
          std::sqrt(std::norm(qxx(i, j)) + std::norm(qyy(i, j)) + real_type(2) * std::norm(qxy(i, j)));
      if (norm > alpha) {
        const real_type scale = alpha / norm;
        qxx(i, j) *= scale;
        qyy(i, j) *= scale;
        qxy(i, j) *= scale;
      }
    }
  }
}

/*! 1D apodisation window sample on an FFTW-ordered axis of length N_lr.
 *  The window is built in fftshift-centred coordinates and mapped back to
 *  FFTW's natural ordering (DC at index 0, negative frequencies in upper half).
 *  Returned values are real-valued, in [0, 1].
 */
inline real_type window_sample_1d(WindowKind kind, Eigen::Index m, Eigen::Index N_lr, real_type tukey_alpha = 0.5) {
  // |k_centred| / (N_lr/2) ∈ [0, 1].
  const Eigen::Index k_abs = std::min(m, N_lr - m);
  const real_type r = real_type(2) * real_type(k_abs) / real_type(N_lr);
  switch (kind) {
  case WindowKind::Rect:
    return real_type(1);
  case WindowKind::Hamming:
    return real_type(0.54) + real_type(0.46) * std::cos(Math::pi * r);
  case WindowKind::Tukey: {
    if (r <= real_type(1) - tukey_alpha)
      return real_type(1);
    if (r >= real_type(1))
      return real_type(0);
    return real_type(0.5) * (real_type(1) + std::cos(Math::pi * (r - (real_type(1) - tukey_alpha)) / tukey_alpha));
  }
  }
  return real_type(1);
}

/*! Build a separable 2D apodisation window of size M_lr × N_lr in FFTW's natural ordering. */
inline void build_window_2d(WindowKind kind, Eigen::Index M_lr, Eigen::Index N_lr, real_matrix &W) {
  W.resize(M_lr, N_lr);
  Eigen::Matrix<real_type, Eigen::Dynamic, 1> w_rows(M_lr);
  Eigen::Matrix<real_type, Eigen::Dynamic, 1> w_cols(N_lr);
  for (Eigen::Index m = 0; m < M_lr; ++m)
    w_rows(m) = window_sample_1d(kind, m, M_lr);
  for (Eigen::Index n = 0; n < N_lr; ++n)
    w_cols(n) = window_sample_1d(kind, n, N_lr);
  for (Eigen::Index j = 0; j < N_lr; ++j)
    for (Eigen::Index i = 0; i < M_lr; ++i)
      W(i, j) = w_rows(i) * w_cols(j);
}

/*! Map a 1D LR k-space index m ∈ [0, N_lr) to its location h(m) in the 2N_lr HR
 *  k-space axis, using FFTW's natural (un-shifted) ordering:
 *    m < N_lr/2 → h(m) = m         (positive frequencies)
 *    m ≥ N_lr/2 → h(m) = m + N_lr  (negative frequencies; LR Nyquist mapped to HR -N_lr/2)
 */
inline Eigen::Index lr_to_hr_index(Eigen::Index m, Eigen::Index N_lr) { return m < N_lr / 2 ? m : m + N_lr; }

/*! Centred frequency for FFTW-ordered index m ∈ [0, N_lr): equals m for the lower half,
 *  and m - N_lr for the upper half (i.e., negative frequencies). For odd N_lr the split
 *  is at floor(N_lr/2); the LR Nyquist (when N_lr is even) is mapped to its negative form. */
inline Eigen::Index centred_frequency(Eigen::Index m, Eigen::Index N_lr) { return m < N_lr / 2 ? m : m - N_lr; }

} // namespace MR::Mrdegibbs2
