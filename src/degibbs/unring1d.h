/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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

#ifndef __degibbs_unring_1d_h__
#define __degibbs_unring_1d_h__

#include "math/fft.h"

namespace MR {
namespace Degibbs {

class Unring1D {
public:
  Unring1D(Math::FFT1D &fft, const int nsh, const int minW, const int maxW)
      : nsh(nsh), minW(minW), maxW(maxW), fft(fft), shifted(fft.size(), 2 * nsh + 1), shifts(2 * nsh + 1) {
    shifts[0] = 0;
    for (int j = 0; j < nsh; j++) {
      shifts[j + 1] = j + 1;
      shifts[1 + nsh + j] = -(j + 1);
    }
  }

  template <typename Derived> FORCE_INLINE void operator()(Eigen::MatrixBase<Derived> &data) {
    assert(data.cols() == 1);
    assert(fft.size() == size_t(data.size()));

    double TV1arr[2 * nsh + 1];
    double TV2arr[2 * nsh + 1];

    const int n = fft.size();
    const int maxn = (n & 1) ? (n - 1) / 2 : n / 2 - 1;

    // iFFT original line as-is:
    for (int i = 0; i < n; ++i)
      fft[i] = data[i];
    fft.run();
    for (int i = 0; i < n; ++i)
      shifted(i, 0) = fft[i];

    // apply shifts and iFFT each line:
    for (int j = 1; j < 2 * nsh + 1; j++) {
      double phi = Math::pi * double(shifts[j]) / double(n * nsh);
      cdouble u(std::cos(phi), std::sin(phi));
      cdouble e(1.0, 0.0);
      fft[0] = data[0];

      if (!(n & 1))
        fft[n / 2] = cdouble(0.0, 0.0);

      for (int l = 0; l < maxn; l++) {
        e = u * e;
        int L = l + 1;
        fft[L] = e * data[L];
        L = n - 1 - l;
        fft[L] = std::conj(e) * data[L];
      }

      fft.run();
      for (int i = 0; i < n; ++i)
        shifted(i, j) = fft[i];
    }

    for (int j = 0; j < 2 * nsh + 1; ++j) {
      TV1arr[j] = 0.0;
      TV2arr[j] = 0.0;
      for (int t = minW; t <= maxW; t++) {
        TV1arr[j] += abs(shifted((n - t) % n, j).real() - shifted((n - t - 1) % n, j).real());
        TV1arr[j] += abs(shifted((n - t) % n, j).imag() - shifted((n - t - 1) % n, j).imag());
        TV2arr[j] += abs(shifted((n + t) % n, j).real() - shifted((n + t + 1) % n, j).real());
        TV2arr[j] += abs(shifted((n + t) % n, j).imag() - shifted((n + t + 1) % n, j).imag());
      }
    }

    for (int l = 0; l < n; ++l) {
      double minTV = std::numeric_limits<double>::max();
      int minidx = 0;
      for (int j = 0; j < 2 * nsh + 1; ++j) {

        if (TV1arr[j] < minTV) {
          minTV = TV1arr[j];
          minidx = j;
        }
        if (TV2arr[j] < minTV) {
          minTV = TV2arr[j];
          minidx = j;
        }

        TV1arr[j] += abs(shifted((l - minW + 1 + n) % n, j).real() - shifted((l - (minW) + n) % n, j).real());
        TV1arr[j] -= abs(shifted((l - maxW + n) % n, j).real() - shifted((l - (maxW + 1) + n) % n, j).real());
        TV2arr[j] += abs(shifted((l + maxW + 1 + n) % n, j).real() - shifted((l + (maxW + 2) + n) % n, j).real());
        TV2arr[j] -= abs(shifted((l + minW + n) % n, j).real() - shifted((l + (minW + 1) + n) % n, j).real());

        TV1arr[j] += abs(shifted((l - minW + 1 + n) % n, j).imag() - shifted((l - (minW) + n) % n, j).imag());
        TV1arr[j] -= abs(shifted((l - maxW + n) % n, j).imag() - shifted((l - (maxW + 1) + n) % n, j).imag());
        TV2arr[j] += abs(shifted((l + maxW + 1 + n) % n, j).imag() - shifted((l + (maxW + 2) + n) % n, j).imag());
        TV2arr[j] -= abs(shifted((l + minW + n) % n, j).imag() - shifted((l + (minW + 1) + n) % n, j).imag());
      }

      double a0r = shifted((l - 1 + n) % n, minidx).real();
      double a1r = shifted(l, minidx).real();
      double a2r = shifted((l + 1 + n) % n, minidx).real();
      double a0i = shifted((l - 1 + n) % n, minidx).imag();
      double a1i = shifted(l, minidx).imag();
      double a2i = shifted((l + 1 + n) % n, minidx).imag();
      double s = double(shifts[minidx]) / (2.0 * nsh);

      if (s > 0.0)
        data(l) = cdouble(a1r * (1.0 - s) + a0r * s, a1i * (1.0 - s) + a0i * s);
      else
        data(l) = cdouble(a1r * (1.0 + s) - a2r * s, a1i * (1.0 + s) - a2i * s);
    }
  }

  template <typename Derived> FORCE_INLINE void operator()(Eigen::MatrixBase<Derived> &&data) { operator()(data); }

  const int nsh, minW, maxW;

private:
  Math::FFT1D &fft;
  Eigen::MatrixXcd shifted;
  vector<int> shifts;
};

} // namespace Degibbs
} // namespace MR

#endif
