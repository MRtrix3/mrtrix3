/* Copyright (c) 2008-2025 the MRtrix3 contributors.
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

#include <vector>

#include "algo/copy.h"
#include "algo/loop.h"
#include "filter/base.h"
#include "filter/kspace.h"
#include "filter/smooth.h"
#include "image.h"
#include "interp/cubic.h"
#include "math/fft.h"
#include "progressbar.h"

namespace MR::Filter {
/** \addtogroup Filters
@{ */

// // From Manzano-Patron et al. 2024
// constexpr default_type default_tukey_FWHM_demodulate = 0.58;
// constexpr default_type default_tukey_alpha_demodulate = 2.0 * (1.0 - default_tukey_FWHM_demodulate);
// Note: Demodulate filter uses Hann window, not tukey

class Demodulate : public Base {
public:
  template <class ImageType>
  Demodulate(ImageType &in, const std::vector<size_t> &inner_axes, const bool linear)
      : Base(in),
        phase(Image<cfloat>::scratch(in,                                     //
                                     std::string("Scratch image storing ")   //
                                         + (linear ? "linear" : "nonlinear") //
                                         + " phase for demodulator")) {      //

    using value_type = typename ImageType::value_type;
    assert(DataType::from<value_type>().is_complex());
    using real_type = typename ImageType::value_type::value_type;

    ImageType input(in);

    std::vector<size_t> outer_axes;
    std::vector<size_t>::const_iterator it = inner_axes.begin();
    for (size_t axis = 0; axis != input.ndim(); ++axis) {
      if (it != inner_axes.end() && *it == axis)
        ++it;
      else
        outer_axes.push_back(axis);
    }

    ProgressBar progress(std::string("estimating ") + (linear ? "linear" : "nonlinear") + " phase modulation",
                         inner_axes.size() + 1 + (linear ? 0 : inner_axes.size()));

    // FFT currently hard-wired to double precision;
    //   have to manually load into cdouble memory
    Image<cdouble> kspace;
    {
      Image<cdouble> temp;
      for (ssize_t n = 0; n != inner_axes.size(); ++n) {
        switch (n) {
        case 0:
          // TODO For now doing a straight copy;
          //   would be preferable if, if the type of the input image is cdouble,
          //   to just make temp a view of input
          // In retrospect it's probably necessary to template the whole k-space derivation
          temp = Image<cdouble>::scratch(in, "Scratch k-space for phase demodulator (1)");
          copy(input, temp);
          kspace = Image<cdouble>::scratch(in, "Scratch k-space for phase demodulator (2)");
          break;
        // TODO If it's possible to prevent the Algo::copy() call above,
        //   then the below code block can be re-introduced
        // case 1:
        // temp = kspace;
        // kspace = Image<cdouble>::scratch(in, "Scratch k-space for phase demodulator (2)");
        // break;
        default:
          std::swap(temp, kspace);
          break;
        }
        // Centred FFT if linear (so we can do peak-finding without wraparound);
        //   not centred if non-linear (for compatibility with window filter generation function)
        Math::FFT(temp, kspace, inner_axes[n], FFTW_FORWARD, linear);
        ++progress;
      }
    }

    auto gen_linear_phase = [&](Image<cdouble> &kspace, Image<cfloat> &phase, const std::vector<size_t> &axes) {
      std::array<bool, 3> axis_mask({false, false, false});
      for (auto axis : axes) {
        if (axis > 2)
          throw Exception("Linear phase demodulator does not support non-spatial inner axes");
        axis_mask[axis] = true;
      }

      std::vector<ssize_t> index_of_max(kspace.ndim());
      std::vector<size_t>::const_iterator it = axes.begin();
      for (ssize_t axis = 0; axis != kspace.ndim(); ++axis) {
        if (it != axes.end() && *it == axis) {
          index_of_max[axis] = -1;
          ++it;
        } else {
          index_of_max[axis] = kspace.index(axis);
        }
      }
      double max_magnitude = -std::numeric_limits<double>::infinity();
      cdouble value_at_max = cdouble(std::numeric_limits<double>::signaling_NaN(),  //
                                     std::numeric_limits<double>::signaling_NaN()); //
      for (auto l_inner = Loop(axes)(kspace); l_inner; ++l_inner) {
        const cdouble value = kspace.value();
        const double magnitude = std::abs(value);
        if (magnitude > max_magnitude) {
          max_magnitude = magnitude;
          value_at_max = value;
          for (auto axis : axes)
            index_of_max[axis] = kspace.index(axis);
        }
      }

      using pos_type = Eigen::Matrix<real_type, 3, 1>;
      using gradient_type = Eigen::Matrix<cdouble, 1, 3>;
      pos_type pos({real_type(index_of_max[0]), real_type(index_of_max[1]), real_type(index_of_max[2])});
      gradient_type gradient;
      cdouble value;
      Interp::SplineInterp<Image<cdouble>,                                 //
                           Math::UniformBSpline<cdouble>,                  //
                           Math::SplineProcessingType::ValueAndDerivative> //
          interp(kspace);                                                  //
      assign_pos_of(index_of_max).to(interp);
      interp.voxel(pos.template cast<double>());
      interp.value_and_gradient(value, gradient);
      const double mag = std::abs(value);
      for (ssize_t iter = 0; iter != 10; ++iter) {
        pos_type grad_mag({0.0, 0.0, 0.0}); // Gradient of the magnitude of the complex k-space data
        for (ssize_t axis = 0; axis != 3; ++axis) {
          if (axis_mask[axis])
            grad_mag[axis] = (value.real() * gradient[axis].real() + value.imag() * gradient[axis].imag()) / mag;
        }
        grad_mag /= max_magnitude;
        pos += 1.0 * grad_mag;
        interp.value_and_gradient(value, gradient);
      }
      value_at_max = value;
      const real_type phase_at_max = std::arg(value_at_max);
      max_magnitude = std::abs(value_at_max);

      // Determine direction and frequency of harmonic
      pos_type kspace_origin;
      for (ssize_t axis = 0; axis != 3; ++axis)
        // Do integer division, then convert to floating-point
        kspace_origin[axis] = axis_mask[axis] ? real_type((kspace.size(axis) - 1) / 2) : real_type(0);
      const pos_type kspace_offset({axis_mask[0] ? (pos[0] - kspace_origin[0]) : real_type(0),
                                    axis_mask[1] ? (pos[1] - kspace_origin[1]) : real_type(0),
                                    axis_mask[2] ? (pos[2] - kspace_origin[2]) : real_type(0)});
      const pos_type cycle_voxel({axis_mask[0] ? (phase.size(0) / kspace_offset[0]) : real_type(0),
                                  axis_mask[1] ? (phase.size(1) / kspace_offset[1]) : real_type(0),
                                  axis_mask[2] ? (phase.size(2) / kspace_offset[2]) : real_type(0)});
      const pos_type voxel_origin({real_type(0.5 * (phase.size(0) - 1.0)),
                                   real_type(0.5 * (phase.size(1) - 1.0)),
                                   real_type(0.5 * (phase.size(2) - 1.0))});
      for (auto l = Loop(axes)(phase); l; ++l) {
        pos = {real_type(phase.index(0)), real_type(phase.index(1)), real_type(phase.index(2))};
        const real_type voxel_phase =
            phase_at_max + (2.0 * Math::pi * (pos - voxel_origin).dot(cycle_voxel) / cycle_voxel.squaredNorm());
        const value_type modulator(std::cos(voxel_phase), std::sin(voxel_phase));
        phase.value() = cfloat(modulator);
      }
    }; // End of gen_linear_phase()

    auto gen_nonlinear_phase = [&](Image<value_type> &input,          //
                                   Image<cdouble> &kspace,            //
                                   Image<cfloat> &phase,              //
                                   const std::vector<size_t> &axes) { //
      Image<double> window = Filter::KSpace::window_hann(input, axes);
      Adapter::Replicate<Image<double>> replicating_window(window, in);
      for (auto l = Loop(kspace)(kspace, replicating_window); l; ++l)
        kspace.value() *= double(replicating_window.value());
      for (auto axis : axes) {
        Math::FFT(kspace, kspace, axis, FFTW_BACKWARD, false);
        ++progress;
      }
      for (auto l = Loop(kspace)(kspace, phase); l; ++l) {
        cdouble value = cdouble(kspace.value());
        value /= std::sqrt(norm(value));
        phase.value() = {float(value.real()), float(value.imag())};
      }
    }; // End of gen_nonlinear_phase()

    if (linear) {
      if (outer_axes.size()) {
        for (auto l_outer = Loop(outer_axes)(kspace, phase); l_outer; ++l_outer)
          gen_linear_phase(kspace, phase, inner_axes);
      } else {
        gen_linear_phase(kspace, phase, inner_axes);
      }
    } else {
      gen_nonlinear_phase(input, kspace, phase, inner_axes);
    }
  }

  template <class InputImageType, class OutputImageType>
  typename std::enable_if<MR::is_complex<typename InputImageType::value_type>::value &&
                              MR::is_complex<typename OutputImageType::value_type>::value,
                          void>::type
  operator()(InputImageType &in, OutputImageType &out, const bool forward = false) {
    if (forward) {
      for (auto l = Loop("re-applying phase modulation")(in, phase, out); l; ++l)
        out.value() = typename OutputImageType::value_type(typename InputImageType::value_type(in.value()) *
                                                           typename InputImageType::value_type(cfloat(phase.value())));
    } else {
      for (auto l = Loop("performing phase demodulation")(in, phase, out); l; ++l)
        out.value() =
            typename OutputImageType::value_type(typename InputImageType::value_type(in.value()) *
                                                 typename InputImageType::value_type(std::conj(cfloat(phase.value()))));
    }
  }

  template <class ImageType>
  typename std::enable_if<MR::is_complex<typename ImageType::value_type>::value, void>::type
  operator()(ImageType &image, const bool forward = false) {
    if (forward) {
      for (auto l = Loop("re-applying phase modulation")(image, phase); l; ++l)
        image.value() = typename ImageType::value_type(typename ImageType::value_type(image.value()) *
                                                       typename ImageType::value_type(cfloat(phase.value())));
    } else {
      for (auto l = Loop("performing phase demodulation")(image, phase); l; ++l)
        image.value() =
            typename ImageType::value_type(typename ImageType::value_type(image.value()) *
                                           typename ImageType::value_type(std::conj(cfloat(phase.value()))));
    }
  }

  Image<cfloat> operator()() const { return phase; }

protected:
  // TODO Change to Image<float>; can produce complex value at processing time
  Image<cfloat> phase;

  // TODO Define a protected class that can be utilised to generate the phase image in a multi-threaded manner
};
//! @}
} // namespace MR::Filter
