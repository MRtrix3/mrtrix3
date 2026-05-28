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

#include <fmt/format.h>
#include <iomanip>
#include <vector>

#include "math/median.h"

namespace MR::Stats {

extern const std::vector<std::string> field_choices;
extern const App::OptionGroup Options;

using value_type = default_type;
using complex_type = cdouble;

//! Online image statistics accumulator.
/*! \a T selects the storage/formatting domain at compile time: \c default_type for
 *  guaranteed-real data (so the complex formatter is never engaged) and \c cdouble for
 *  complex data, where min/max/std are tracked separately for real and imaginary parts. */
template <typename T> class Stats {
public:
  Stats(const bool ignorezero = false) : count(0), ignore_zero(ignorezero) {
    if constexpr (is_complex<T>::value) {
      mean = delta = delta2 = m2 = std = T(0.0, 0.0);
      min = T(Inf, Inf);
      max = T(-Inf, -Inf);
    } else {
      mean = delta = delta2 = m2 = std = T(0);
      min = Inf;
      max = -Inf;
    }
    std_rv = value_type(0);
  }

  void operator()(T val);

  template <class ImageType> void print(ImageType &ima, const std::vector<std::string> &fields) {

    if (count > 1) {
      if constexpr (is_complex<T>::value) {
        std = T(sqrt(m2.real() / static_cast<value_type>(count - 1)),
                sqrt(m2.imag() / static_cast<value_type>(count - 1)));
        std_rv = sqrt((m2.real() + m2.imag()) / static_cast<value_type>(count - 1));
      } else {
        std = std::sqrt(m2 / static_cast<value_type>(count - 1));
        std_rv = std;
      }
      std::sort(values.begin(), values.end());
    }
    if (!fields.empty()) {
      if (!count) {
        if (fields.size() == 1 && fields.front() == "count") {
          std::cout << "0\n";
          return;
        } else {
          throw Exception("Cannot output statistic of interest; no values read (empty mask?)");
        }
      }
      for (size_t n = 0; n < fields.size(); ++n) {
        if (fields[n] == "mean")
          std::cout << fmt::format("{}", mean) << " ";
        else if (fields[n] == "median")
          std::cout << (!values.empty() ? fmt::format("{}", Math::median(values)) : "N/A") << " ";
        else if (fields[n] == "std")
          std::cout << (count > 1 ? fmt::format("{}", std) : "N/A") << " ";
        else if (fields[n] == "std_rv")
          std::cout << (count > 1 ? fmt::format("{}", std_rv) : "N/A") << " ";
        else if (fields[n] == "iqr")
          std::cout << (!values.empty() ? fmt::format("{}", Math::quantile(values, 0.75) - Math::quantile(values, 0.25))
                                        : "N/A")
                    << " ";
        else if (fields[n] == "min")
          std::cout << fmt::format("{}", min) << " ";
        else if (fields[n] == "max")
          std::cout << fmt::format("{}", max) << " ";
        else if (fields[n] == "count")
          std::cout << count << " ";
        else
          throw Exception("stats type not supported: {}", fields[n]);
      }
      std::cout << "\n";

    } else {

      std::string s = "[ ";
      if (ima.ndim() > 3)
        for (size_t n = 3; n < ima.ndim(); n++)
          s += fmt::format("{} ", ima.index(n));
      else
        s += "0 ";
      s += "]";

      constexpr int width = is_complex<T>::value ? 20 : 10;
      std::cout << std::setw(12) << std::right << s << " ";

      std::cout << std::setw(width) << std::right << (count ? fmt::format("{}", mean) : "N/A");

      if constexpr (!is_complex<T>::value) {
        std::cout << " " << std::setw(width) << std::right;
        if (count)
          std::cout << fmt::format("{}", Math::median(values));
        else
          std::cout << "N/A";
      }
      std::cout << " " << std::setw(width) << std::right << (count > 1 ? fmt::format("{}", std) : "N/A") << " "
                << std::setw(width) << std::right << (count ? fmt::format("{}", min) : "N/A") << " " << std::setw(width)
                << std::right << (count ? fmt::format("{}", max) : "N/A") << " " << std::setw(10) << std::right << count
                << "\n";
    }
  }

private:
  T mean, delta, delta2, m2, std, min, max;
  value_type std_rv;
  size_t count;
  const bool ignore_zero;
  std::vector<float> values;
};

template <typename T> void print_header() {
  constexpr int width = is_complex<T>::value ? 20 : 10;
  std::cout << std::setw(12) << std::right << "volume"
            << " " << std::setw(width) << std::right << "mean";
  if constexpr (!is_complex<T>::value)
    std::cout << " " << std::setw(width) << std::right << "median";
  std::cout << " " << std::setw(width) << std::right << "std"
            << " " << std::setw(width) << std::right << "min"
            << " " << std::setw(width) << std::right << "max"
            << " " << std::setw(10) << std::right << "count"
            << "\n";
}

} // namespace MR::Stats
