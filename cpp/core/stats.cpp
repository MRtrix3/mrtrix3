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

#include "stats.h"

namespace MR::Stats {

using namespace App;

const std::vector<std::string> field_choices{"mean", "median", "std", "std_rv", "iqr", "min", "max", "count"};

// clang-format off
const OptionGroup Options =
    OptionGroup("Statistics options")
    + Option("output",
             fmt::format("output only the field specified."
                         " Multiple such options can be supplied if required."
                         " Choices are: {}."
                         " Useful for use in scripts."
                         " Both std options refer to the unbiased (sample) standard deviation."
                         " For complex data, min, max and std are calculated separately for real and imaginary parts,"
                         " std_rv is based on the real valued variance"
                         " (equals sqrt of sum of variances of imaginary and real parts).",
                         join(field_choices, ", "))).allow_multiple()
      + Argument("field").type_choice(field_choices)
    + Option("mask",
             "only perform computation within the specified binary mask image.")
      + Argument("image").type_image_in()
    + Option("ignorezero",
             "ignore zero values during statistics calculation");
// clang-format on

template <typename T> void Stats<T>::operator()(T val) {
  if constexpr (is_complex<T>::value) {
    if (std::isfinite(val.real()) && std::isfinite(val.imag()) &&
        (!ignore_zero || val.real() != 0.0 || val.imag() != 0.0)) {
      if (min.real() > val.real())
        min = T(val.real(), min.imag());
      if (min.imag() > val.imag())
        min = T(min.real(), val.imag());
      if (max.real() < val.real())
        max = T(val.real(), max.imag());
      if (max.imag() < val.imag())
        max = T(max.real(), val.imag());
      count++;
      // Welford's online algorithm for variance calculation:
      delta = val - mean;
      mean += T{delta.real() / static_cast<double>(count), delta.imag() / static_cast<double>(count)};
      delta2 = val - mean;
      m2 += T{delta.real() * delta2.real(), delta.imag() * delta2.imag()};
    }
  } else {
    if (std::isfinite(val) && (!ignore_zero || val != 0.0)) {
      if (min > val)
        min = val;
      if (max < val)
        max = val;
      count++;
      // Welford's online algorithm for variance calculation:
      delta = val - mean;
      mean += delta / static_cast<double>(count);
      delta2 = val - mean;
      m2 += delta * delta2;
      values.push_back(static_cast<value_type>(val));
    }
  }
}

template class Stats<value_type>;
template class Stats<complex_type>;

} // namespace MR::Stats
