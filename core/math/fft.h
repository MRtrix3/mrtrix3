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
#ifndef __math_fft_h__
#define __math_fft_h__

#include "image.h"
#include <fftw3.h>

namespace MR {
namespace Math {

//! a class to perform an in-place 1D FFT
/*! This class expects its data buffer of size \a N to be filled in using
 * its operator[] method, then the run() method can be invoked, and the
 * results read out using the same operator[] method.
 *
 * The \a direction parameter should be either FFTW_FORWARD or
 * FFTW_BACKWARD */
class FFT1D {
public:
  FFT1D(size_t N, int direction) : _data(N), direction(direction) {
    fftw_complex *p = reinterpret_cast<fftw_complex *>(&(_data[0]));
    _plan = fftw_plan_dft_1d(N, p, p, direction, FFTW_MEASURE);
  }

  FFT1D(const FFT1D &other) : _data(other._data.size()), direction(other.direction) {
    fftw_complex *p = reinterpret_cast<fftw_complex *>(&(_data[0]));
    _plan = fftw_plan_dft_1d(other._data.size(), p, p, direction, FFTW_MEASURE);
  }

  const size_t size() const { return _data.size(); }

  const cdouble &operator[](size_t n) const { return _data[n]; }
  cdouble &operator[](size_t n) { return _data[n]; }

  void run() { fftw_execute(_plan); }

protected:
  Eigen::VectorXcd _data;
  fftw_plan _plan;
  int direction;
};

namespace {
inline std::string direction_str(int direction) { return (direction == FFTW_FORWARD ? "forward" : "backward"); }

inline size_t shift(size_t pos, const size_t size, const bool centre_FFT, const bool inverse) {
  if (centre_FFT) {
    pos += (size + 1 + (!inverse)) / 2;
    if (pos >= size)
      pos -= size;
  }
  return pos;
}
} // namespace

//! perform FFT of input image \a in along specified axis, writing results into output image \a out
/* The \a direction parameter should be either FFTW_FORWARD or
 * FFTW_BACKWARD.
 *
 * The output image \a out can be the same as the input \a in, in which
 * case the FFT will be performed in-place.  */
template <class ImageTypeIn, class ImageTypeOut>
inline void FFT(ImageTypeIn &in, ImageTypeOut &out, size_t axis, int direction, bool centre_FFT = false) {
  class FFTFunctor {
  public:
    FFTFunctor(const ImageTypeIn &in, const ImageTypeOut &out, size_t axis, int direction, bool centre_FFT)
        : in(in), out(out), fft(in.size(axis), direction), axis(axis), direction(direction), centre_FFT(centre_FFT) {}

    void operator()(const Iterator &pos) {
      assign_pos_of(pos).to(in, out);
      for (auto l = Loop(axis, axis + 1)(in); l; ++l)
        fft[shift(in.index(axis), in.size(axis), centre_FFT, direction == FFTW_FORWARD)] = cdouble(in.value());
      fft.run();
      for (auto l = Loop(axis, axis + 1)(out); l; ++l)
        out.value() = fft[shift(out.index(axis), out.size(axis), centre_FFT, direction == FFTW_BACKWARD)];
    }

  protected:
    ImageTypeIn in;
    ImageTypeOut out;
    FFT1D fft;
    size_t axis;
    const int direction;
    const bool centre_FFT;
  };

  auto outer_axes = Stride::order(in);
  outer_axes.erase(std::find(outer_axes.begin(), outer_axes.end(), axis));
  outer_axes.insert(outer_axes.begin(), axis);

  ThreadedLoop("performing " + direction_str(direction) + " FFT along axis " + str(axis), in, outer_axes, 1)
      .run_outer(FFTFunctor(in, out, axis, direction, centre_FFT));
}

} // namespace Math
} // namespace MR

#endif
