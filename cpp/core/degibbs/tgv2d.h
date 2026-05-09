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
#include <cmath>
#include <fftw3.h>
#include <vector>

#include "algo/iterator.h"
#include "algo/loop.h"
#include "algo/threaded_loop.h"
#include "degibbs/degibbs.h"
#include "degibbs/operators2d.h"
#include "exception.h"
#include "image.h"
#include "image_helpers.h"
#include "math/math.h"

namespace MR::Mrdegibbs2 {

/*! Per-slice Chambolle-Pock primal-dual solver for the super-resolution +
 *  TGV² recovery problem
 *
 *  \f$ \min_{u} \tfrac{1}{2} \|A u - b\|_2^2 + \mu \cdot \mathrm{TGV}^2_{\alpha_1,\alpha_0}(u) \f$
 *
 *  with the data operator \f$ A u = M_\Phi \circ M_W \circ \mathrm{crop} \circ \hat{\mathcal F}_{HR}(u) \f$
 *  and \f$ b = 2 \cdot \hat{\mathcal F}_{LR}(y) \f$, where:
 *    - \f$ \hat{\mathcal F}_{HR}, \hat{\mathcal F}_{LR} \f$ are unitary Fourier transforms
 *      on the HR (\f$2M_{lr}\times2N_{lr}\f$) and LR (\f$M_{lr}\times N_{lr}\f$) grids;
 *    - crop selects the central LR k-space window from HR k-space;
 *    - \f$ M_W \f$ is multiplication by the (real, separable) apodisation window;
 *    - \f$ M_\Phi \f$ is multiplication by the per-frequency phase
 *      \f$ \Phi(m_x,m_y) = \exp(+i\pi(m_x^c/(2M_{lr}) + m_y^c/(2N_{lr}))) \f$ that
 *      compensates the half-sample offset between the LR sampling grid (origin at
 *      the LR voxel centre) and the half-shifted HR sampling grid (origin shifted
 *      by \f$ -\tfrac{1}{2}\Delta x_{HR} \f$ per axis so that 2×2 HR voxel
 *      aggregation lands at the LR voxel positions).
 *      Without this phase the IFFT would put bandlimited content at no-shift HR
 *      positions while the affine declares HR voxels at half-shifted positions,
 *      producing a half-HR-voxel content drift toward the voxel-space origin.
 *
 *  \f$ A^* A \f$ is diagonal in HR k-space (= \f$|W|^2\f$ inside the LR window,
 *  zero outside), so the data-term primal proximal step reduces to one HR FFT,
 *  per-frequency arithmetic, and one inverse HR FFT — no sparse linear solver.
 *
 *  TGV² is unrolled to its lifted (Bredies/Kunisch/Pock) form by introducing
 *  the auxiliary vector field \f$ v \f$, yielding the saddle point
 *  \f$ \min_{u,v} \max_{p,q} \langle \nabla u - v, p \rangle +
 *  \langle \epsilon(v), q \rangle + \tfrac{1}{2}\|Au - b\|^2
 *  - \alpha_1 \delta_{\|p\|\le 1}(\cdot) - \alpha_0 \delta_{\|q\|\le 1}(\cdot) \f$.
 *
 *  Step sizes are set from the safe bound \f$ \|K\|^2 \le 12 \f$.
 *  The L1 norms are coupled across (gradient direction, real/imag) — the
 *  isotropic / channel-joint convention of Knoll et al. 2011.
 *  An alternative is to project real and imaginary parts independently
 *  (decoupled); see project_grad_dual / project_sym_dual in operators2d.h. */
class TGV2DSolver {
public:
  TGV2DSolver(Eigen::Index M_lr_,
              Eigen::Index N_lr_,
              real_type mu,
              real_type alpha1_in,
              real_type alpha0_in,
              WindowKind window_kind,
              int max_iter_,
              real_type tol_)
      : M_lr(M_lr_),
        N_lr(N_lr_),
        M_hr(2 * M_lr_),
        N_hr(2 * N_lr_),
        alpha1(mu * alpha1_in),
        alpha0(mu * alpha0_in),
        max_iter(max_iter_),
        tol(tol_),
        sigma(real_type(1) / std::sqrt(real_type(12))),
        tau(real_type(1) / std::sqrt(real_type(12))),
        hr_buffer(M_hr, N_hr),
        lr_buffer(M_lr, N_lr) {
    if (M_lr < 4 || N_lr < 4)
      throw Exception("TGV2D solver requires both in-plane sizes to be at least 4");

    build_window_2d(window_kind, M_lr, N_lr, W_lr);

    // Half-sample-shift phase, conjugate (used in additive term and init).
    phase_conj_lr.resize(M_lr, N_lr);
    for (Eigen::Index j = 0; j < N_lr; ++j) {
      const Eigen::Index my_c = centred_frequency(j, N_lr);
      for (Eigen::Index i = 0; i < M_lr; ++i) {
        const Eigen::Index mx_c = centred_frequency(i, M_lr);
        const real_type angle = -Math::pi * (real_type(mx_c) / real_type(2 * M_lr) //
                                             + real_type(my_c) / real_type(2 * N_lr));
        phase_conj_lr(i, j) = complex_type(std::cos(angle), std::sin(angle));
      }
    }

    // Pre-compute the HR-k-space prox denominator at LR-window positions.
    denom_lr.resize(M_lr, N_lr);
    for (Eigen::Index j = 0; j < N_lr; ++j)
      for (Eigen::Index i = 0; i < M_lr; ++i)
        denom_lr(i, j) = real_type(1) + tau * W_lr(i, j) * W_lr(i, j);

    // FFTW dimensions: for an Eigen column-major buffer of size (rows=M, cols=N),
    // memory order is fastest along rows, so FFTW (which reads in row-major) needs
    // dimensions (n0=N, n1=M) — i.e. cols first, rows second.
    fftw_complex *p_hr = reinterpret_cast<fftw_complex *>(hr_buffer.data());
    plan_fwd_hr =
        fftw_plan_dft_2d(static_cast<int>(N_hr), static_cast<int>(M_hr), p_hr, p_hr, FFTW_FORWARD, FFTW_MEASURE);
    plan_bwd_hr =
        fftw_plan_dft_2d(static_cast<int>(N_hr), static_cast<int>(M_hr), p_hr, p_hr, FFTW_BACKWARD, FFTW_MEASURE);
    fftw_complex *p_lr = reinterpret_cast<fftw_complex *>(lr_buffer.data());
    plan_fwd_lr =
        fftw_plan_dft_2d(static_cast<int>(N_lr), static_cast<int>(M_lr), p_lr, p_lr, FFTW_FORWARD, FFTW_MEASURE);
  }

  /*! Copy constructor — recreates FFTW plans on fresh buffers for use by a
   *  worker thread copy. FFTW plan creation is not thread-safe, so this should
   *  only be invoked sequentially on the main thread (as ThreadedLoop does
   *  when populating per-thread functor copies via std::vector). */
  TGV2DSolver(const TGV2DSolver &other)
      : M_lr(other.M_lr),
        N_lr(other.N_lr),
        M_hr(other.M_hr),
        N_hr(other.N_hr),
        alpha1(other.alpha1),
        alpha0(other.alpha0),
        max_iter(other.max_iter),
        tol(other.tol),
        sigma(other.sigma),
        tau(other.tau),
        W_lr(other.W_lr),
        phase_conj_lr(other.phase_conj_lr),
        denom_lr(other.denom_lr),
        hr_buffer(M_hr, N_hr),
        lr_buffer(M_lr, N_lr) {
    fftw_complex *p_hr = reinterpret_cast<fftw_complex *>(hr_buffer.data());
    plan_fwd_hr =
        fftw_plan_dft_2d(static_cast<int>(N_hr), static_cast<int>(M_hr), p_hr, p_hr, FFTW_FORWARD, FFTW_MEASURE);
    plan_bwd_hr =
        fftw_plan_dft_2d(static_cast<int>(N_hr), static_cast<int>(M_hr), p_hr, p_hr, FFTW_BACKWARD, FFTW_MEASURE);
    fftw_complex *p_lr = reinterpret_cast<fftw_complex *>(lr_buffer.data());
    plan_fwd_lr =
        fftw_plan_dft_2d(static_cast<int>(N_lr), static_cast<int>(M_lr), p_lr, p_lr, FFTW_FORWARD, FFTW_MEASURE);
  }

  ~TGV2DSolver() {
    fftw_destroy_plan(plan_fwd_hr);
    fftw_destroy_plan(plan_bwd_hr);
    fftw_destroy_plan(plan_fwd_lr);
  }

  TGV2DSolver &operator=(const TGV2DSolver &) = delete;
  TGV2DSolver(TGV2DSolver &&) = delete;
  TGV2DSolver &operator=(TGV2DSolver &&) = delete;

  /*! Solve for u_hr (size M_hr × N_hr) given the LR slice y_lr (size M_lr × N_lr). */
  void solve(const complex_matrix &y_lr, complex_matrix &u_hr) {
    assert(y_lr.rows() == M_lr && y_lr.cols() == N_lr);

    // 1) LR FFT of input slice (unscaled FFTW output stored in lr_buffer).
    lr_buffer = y_lr;
    fftw_execute(plan_fwd_lr);

    // 2) Per-slice precomputation of the HR-k-space additive term, in unscaled
    //    FFTW units: additive_lr[m] = 4τ · W[m] · phasē[m] · Y_un[m].
    additive_lr.resize(M_lr, N_lr);
    for (Eigen::Index j = 0; j < N_lr; ++j)
      for (Eigen::Index i = 0; i < M_lr; ++i)
        additive_lr(i, j) = real_type(4) * tau * W_lr(i, j) * phase_conj_lr(i, j) * lr_buffer(i, j);

    // 3) Initialise u from the bandlimited zero-pad LR-to-HR interpolation, on the
    //    half-shifted HR sampling grid. HR k-space at LR-window position [h_x,h_y]
    //    = 4 · phasē[m] · Y_un[m] (factor 4 = (M_hr/M_lr) · (N_hr/N_lr) = 4 in 2D);
    //    inverse HR FFT then divide by M_hr · N_hr to undo FFTW's unscaled iDFT.
    hr_buffer.setZero();
    for (Eigen::Index mj = 0; mj < N_lr; ++mj) {
      const Eigen::Index hj = lr_to_hr_index(mj, N_lr);
      for (Eigen::Index mi = 0; mi < M_lr; ++mi) {
        const Eigen::Index hi = lr_to_hr_index(mi, M_lr);
        hr_buffer(hi, hj) = real_type(4) * phase_conj_lr(mi, mj) * lr_buffer(mi, mj);
      }
    }
    fftw_execute(plan_bwd_hr);
    const real_type N_hr_total = real_type(M_hr) * real_type(N_hr);
    hr_buffer /= N_hr_total;

    u = hr_buffer;
    u_bar = u;
    u_prev.resize(M_hr, N_hr);
    vx.setZero(M_hr, N_hr);
    vy.setZero(M_hr, N_hr);
    vx_bar.setZero(M_hr, N_hr);
    vy_bar.setZero(M_hr, N_hr);
    vx_prev.resize(M_hr, N_hr);
    vy_prev.resize(M_hr, N_hr);
    px.setZero(M_hr, N_hr);
    py.setZero(M_hr, N_hr);
    qxx.setZero(M_hr, N_hr);
    qyy.setZero(M_hr, N_hr);
    qxy.setZero(M_hr, N_hr);

    // 4) Chambolle-Pock iteration.
    real_type prev_norm = u.norm();
    for (int iter = 0; iter < max_iter; ++iter) {
      // p ← proj_{α1}( p + σ (∇ū − v̄) )
      grad_op(u_bar, scratch_a, scratch_b);
      px += sigma * (scratch_a - vx_bar);
      py += sigma * (scratch_b - vy_bar);
      project_grad_dual(px, py, alpha1);

      // q ← proj_{α0}( q + σ ε(v̄) )
      sym_grad_op(vx_bar, vy_bar, scratch_a, scratch_b, scratch_c);
      qxx += sigma * scratch_a;
      qyy += sigma * scratch_b;
      qxy += sigma * scratch_c;
      project_sym_dual(qxx, qyy, qxy, alpha0);

      // u_new = prox_{τ G_data}( u + τ div(p) )
      div_op(px, py, scratch_a);
      u_prev = u;
      u += tau * scratch_a;
      apply_data_prox(u);

      // v_new = v + τ ( p + sym_div(q) )
      sym_div_op(qxx, qyy, qxy, scratch_a, scratch_b);
      vx_prev = vx;
      vy_prev = vy;
      vx += tau * (px + scratch_a);
      vy += tau * (py + scratch_b);

      // Over-relaxation.
      u_bar = real_type(2) * u - u_prev;
      vx_bar = real_type(2) * vx - vx_prev;
      vy_bar = real_type(2) * vy - vy_prev;

      // Cheap convergence check on relative change of u.
      if ((iter + 1) % 10 == 0) {
        const real_type cur_norm = u.norm();
        const real_type rel = std::fabs(cur_norm - prev_norm) / std::max(cur_norm, real_type(1e-30));
        if (rel < tol)
          break;
        prev_norm = cur_norm;
      }
    }

    u_hr = u;
  }

  Eigen::Index hr_rows() const { return M_hr; }
  Eigen::Index hr_cols() const { return N_hr; }
  Eigen::Index lr_rows() const { return M_lr; }
  Eigen::Index lr_cols() const { return N_lr; }

private:
  /*! Apply the data-term proximal in HR k-space (diagonal there).
   *  z ← (I + τ A*A)^{-1} (z + τ A* b), implemented per-frequency. */
  void apply_data_prox(complex_matrix &z) {
    hr_buffer = z;
    fftw_execute(plan_fwd_hr);
    for (Eigen::Index mj = 0; mj < N_lr; ++mj) {
      const Eigen::Index hj = lr_to_hr_index(mj, N_lr);
      for (Eigen::Index mi = 0; mi < M_lr; ++mi) {
        const Eigen::Index hi = lr_to_hr_index(mi, M_lr);
        hr_buffer(hi, hj) = (hr_buffer(hi, hj) + additive_lr(mi, mj)) / denom_lr(mi, mj);
      }
    }
    fftw_execute(plan_bwd_hr);
    z = hr_buffer / (real_type(M_hr) * real_type(N_hr));
  }

  Eigen::Index M_lr, N_lr;
  Eigen::Index M_hr, N_hr;
  real_type alpha1, alpha0;
  int max_iter;
  real_type tol;
  real_type sigma, tau;

  complex_matrix u, u_prev, u_bar;
  complex_matrix vx, vy, vx_prev, vy_prev, vx_bar, vy_bar;
  complex_matrix px, py;
  complex_matrix qxx, qyy, qxy;
  complex_matrix scratch_a, scratch_b, scratch_c;

  real_matrix W_lr;
  complex_matrix phase_conj_lr;
  real_matrix denom_lr;
  complex_matrix additive_lr;

  complex_matrix hr_buffer;
  complex_matrix lr_buffer;
  fftw_plan plan_fwd_hr;
  fftw_plan plan_bwd_hr;
  fftw_plan plan_fwd_lr;
};

/*! 2×2 (per axis) average of the HR slice into the LR slice — direct voxel
 *  aggregation. With the half-shifted HR sampling grid produced by the solver,
 *  each averaged 2×2 block of HR voxels lands at exactly the LR voxel position. */
inline void downsample_2x2(const complex_matrix &hr, complex_matrix &lr) {
  const Eigen::Index M_lr = hr.rows() / 2;
  const Eigen::Index N_lr = hr.cols() / 2;
  assert(hr.rows() == 2 * M_lr && hr.cols() == 2 * N_lr);
  lr.resize(M_lr, N_lr);
  for (Eigen::Index j = 0; j < N_lr; ++j)
    for (Eigen::Index i = 0; i < M_lr; ++i)
      lr(i, j) =
          real_type(0.25) * (hr(2 * i, 2 * j) + hr(2 * i + 1, 2 * j) + hr(2 * i, 2 * j + 1) + hr(2 * i + 1, 2 * j + 1));
}

/*! Per-thread functor that applies TGV2DSolver to one 2D slice.
 *
 *  Holds its own solver (and its FFTW plans / scratch buffers); copies of this
 *  functor — one per worker thread — are produced by ThreadedLoop. The solver's
 *  copy constructor allocates fresh FFTW plans so the per-thread state is
 *  fully independent. */
class TGV2DFunctor {
public:
  TGV2DFunctor(const std::vector<size_t> &outer_axes,
               const std::vector<size_t> &slice_axes,
               Image<complex_type> &in,
               Image<complex_type> &out,
               real_type mu,
               real_type alpha1,
               real_type alpha0,
               WindowKind window_kind,
               int max_iter,
               real_type tol,
               bool downsample)
      : outer_axes(outer_axes),
        slice_axes(slice_axes),
        in(in),
        out(out),
        downsample(downsample),
        slice_lr_in(in.size(slice_axes[0]), in.size(slice_axes[1])),
        slice_hr(2 * in.size(slice_axes[0]), 2 * in.size(slice_axes[1])),
        solver(static_cast<Eigen::Index>(in.size(slice_axes[0])),
               static_cast<Eigen::Index>(in.size(slice_axes[1])),
               mu,
               alpha1,
               alpha0,
               window_kind,
               max_iter,
               tol) {
    if (downsample)
      slice_lr_out.resize(in.size(slice_axes[0]), in.size(slice_axes[1]));
  }

  void operator()(const Iterator &pos) {
    const size_t X = slice_axes[0];
    const size_t Y = slice_axes[1];
    assign_pos_of(pos, outer_axes).to(in, out);

    for (auto inner = Loop(slice_axes)(in); inner; ++inner)
      slice_lr_in(static_cast<Eigen::Index>(in.index(X)), static_cast<Eigen::Index>(in.index(Y))) = in.value();

    solver.solve(slice_lr_in, slice_hr);

    if (downsample) {
      downsample_2x2(slice_hr, slice_lr_out);
      for (auto inner = Loop(slice_axes)(out); inner; ++inner)
        out.value() = slice_lr_out(static_cast<Eigen::Index>(out.index(X)), static_cast<Eigen::Index>(out.index(Y)));
    } else {
      for (auto inner = Loop(slice_axes)(out); inner; ++inner)
        out.value() = slice_hr(static_cast<Eigen::Index>(out.index(X)), static_cast<Eigen::Index>(out.index(Y)));
    }
  }

protected:
  const std::vector<size_t> &outer_axes;
  const std::vector<size_t> &slice_axes;
  Image<complex_type> in, out;
  const bool downsample;
  complex_matrix slice_lr_in;
  complex_matrix slice_hr;
  complex_matrix slice_lr_out;
  TGV2DSolver solver;
};

/*! Drive per-volume application of TGV2DSolver to each 2D slice in turn, with
 *  multi-threaded dispatch over the outer axes via ThreadedLoop.
 *
 *  If \a downsample is true, the output image must have the same in-plane sizes
 *  as the input, and each solved HR slice is aggregated 2×2 back to LR before
 *  being written. Otherwise the output must have in-plane sizes exactly twice
 *  those of the input, and HR slices are written directly. */
inline void run_tgv2d(Image<complex_type> &in,
                      Image<complex_type> &out,
                      const std::vector<size_t> &outer_axes,
                      const std::vector<size_t> &slice_axes,
                      real_type mu,
                      real_type alpha1,
                      real_type alpha0,
                      WindowKind window_kind,
                      int max_iter,
                      real_type tol,
                      bool downsample) {
  assert(slice_axes.size() == 2);
  const size_t X = slice_axes[0];
  const size_t Y = slice_axes[1];

  if (downsample) {
    if (out.size(X) != in.size(X) || out.size(Y) != in.size(Y))
      throw Exception("output image in-plane sizes must equal the input when downsampling");
  } else {
    if (out.size(X) != 2 * in.size(X) || out.size(Y) != 2 * in.size(Y))
      throw Exception("output image in-plane sizes must be exactly twice the input when in super-resolution mode");
  }

  ThreadedLoop("performing TGV2 super-resolution Gibbs ringing removal", in, outer_axes, slice_axes)
      .run_outer(
          TGV2DFunctor(outer_axes, slice_axes, in, out, mu, alpha1, alpha0, window_kind, max_iter, tol, downsample));
}

} // namespace MR::Mrdegibbs2
