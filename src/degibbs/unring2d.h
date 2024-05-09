/* Copyright (c) 2008-2024 the MRtrix3 contributors.
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

#include "algo/threaded_loop.h"
#include "axes.h"
#include "degibbs/unring1d.h"
#include "image.h"
#include "progressbar.h"

namespace MR::Degibbs {

typedef cdouble value_type;

class Unring2D {
public:
  Unring2D(size_t nrows, size_t ncols, const int nsh, const int minW, const int maxW)
      : row_fft(ncols, FFTW_FORWARD),
        col_fft(nrows, FFTW_FORWARD),
        row_ifft(ncols, FFTW_BACKWARD),
        col_ifft(nrows, FFTW_BACKWARD),
        unring1d_row(row_ifft, nsh, minW, maxW),
        unring1d_col(col_ifft, nsh, minW, maxW),
        slice2(nrows, ncols) {}

  Unring2D(const Unring2D &other)
      : row_fft(other.row_fft.size(), FFTW_FORWARD),
        col_fft(other.col_fft.size(), FFTW_FORWARD),
        row_ifft(other.row_ifft.size(), FFTW_BACKWARD),
        col_ifft(other.col_ifft.size(), FFTW_BACKWARD),
        unring1d_row(row_ifft, other.unring1d_row.nsh, other.unring1d_row.minW, other.unring1d_row.maxW),
        unring1d_col(col_ifft, other.unring1d_col.nsh, other.unring1d_col.minW, other.unring1d_col.maxW),
        slice2(other.slice2.rows(), other.slice2.cols()) {}

  template <typename Derived> FORCE_INLINE void operator()(Eigen::MatrixBase<Derived> &slice) {
    assert(slice.cols() == slice2.cols());
    assert(slice.rows() == slice2.rows());

    row_FFT(slice);
    col_FFT(slice);

    for (int k = 0; k < slice.cols(); k++) {
      double ck = (1.0 + cos(2.0 * Math::pi * (double(k) / slice.cols()))) * 0.5;
      for (int j = 0; j < slice.rows(); j++) {
        double cj = (1.0 + cos(2.0 * Math::pi * (double(j) / slice.rows()))) * 0.5;

        if (ck + cj != 0.0) {
          slice2(j, k) = slice(j, k) * cj / (ck + cj);
          slice(j, k) *= ck / (ck + cj);
        } else
          slice(j, k) = slice2(j, k) = cdouble(0.0, 0.0);
      }
    }

    row_iFFT(slice);
    col_iFFT(slice2);

    for (ssize_t n = 0; n < slice.cols(); ++n)
      unring1d_col(slice.col(n));
    for (ssize_t n = 0; n < slice2.rows(); ++n)
      unring1d_row(slice2.row(n).transpose());

    slice.noalias() = (slice + slice2) / (slice.rows() * slice.cols());
  }

private:
  Math::FFT1D row_fft, col_fft, row_ifft, col_ifft;
  Unring1D unring1d_row, unring1d_col;
  Eigen::MatrixXcd slice2;

  template <typename fft_obj, typename Derived> FORCE_INLINE void FFT(fft_obj &fft, Eigen::MatrixBase<Derived> &M) {
    assert(fft.size() == size_t(M.cols()));
    for (auto n = 0; n < M.rows(); ++n) {
      for (ssize_t i = 0; i < M.cols(); ++i)
        fft[i] = M(n, i);
      fft.run();
      for (ssize_t i = 0; i < M.cols(); ++i)
        M(n, i) = fft[i];
    }
  }
  template <typename Derived> FORCE_INLINE void FFT(Math::FFT1D &fft, Eigen::MatrixBase<Derived> &&M) { FFT(fft, M); }

  template <typename Derived> FORCE_INLINE void row_FFT(Eigen::MatrixBase<Derived> &mat) { FFT(row_fft, mat); }
  template <typename Derived> FORCE_INLINE void row_iFFT(Eigen::MatrixBase<Derived> &mat) { FFT(row_ifft, mat); }
  template <typename Derived> FORCE_INLINE void col_FFT(Eigen::MatrixBase<Derived> &mat) {
    FFT(col_fft, mat.transpose());
  }
  template <typename Derived> FORCE_INLINE void col_iFFT(Eigen::MatrixBase<Derived> &mat) {
    FFT(col_ifft, mat.transpose());
  }
};

class Unring2DFunctor {
public:
  Unring2DFunctor(const std::vector<size_t> &outer_axes,
                  const std::vector<size_t> &slice_axes,
                  const int &nsh,
                  const int &minW,
                  const int &maxW,
                  Image<value_type> &in,
                  Image<value_type> &out)
      : outer_axes(outer_axes),
        slice_axes(slice_axes),
        in(in),
        out(out),
        slice(in.size(slice_axes[0]), in.size(slice_axes[1])),
        unring2d(in.size(slice_axes[0]), in.size(slice_axes[1]), nsh, minW, maxW) {}

  void operator()(const Iterator &pos) {
    const int X = slice_axes[0];
    const int Y = slice_axes[1];
    assign_pos_of(pos, outer_axes).to(in, out);

    for (auto l = Loop(slice_axes)(in); l; ++l)
      slice(ssize_t(in.index(X)), ssize_t(in.index(Y))) = in.value();

    unring2d(slice);

    for (auto l = Loop(slice_axes)(out); l; ++l)
      out.value() = slice(ssize_t(out.index(X)), ssize_t(out.index(Y)));
  }

protected:
  const std::vector<size_t> &outer_axes;
  const std::vector<size_t> &slice_axes;
  Image<value_type> in, out;
  Eigen::MatrixXcd slice;
  Unring2D unring2d;
};

} // namespace MR::Degibbs
