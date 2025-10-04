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

#include "math/median.h"

#include <iomanip>
#include <vector>

namespace MR::Stats {

extern std::vector<std::string> field_choices;
extern const App::OptionGroup Options;

using value_type = default_type;
using complex_type = cdouble;

class Stats {
public:
  Stats(const bool is_complex = false, const bool ignorezero = false)
      : mean(0.0, 0.0),
        delta(0.0, 0.0),
        delta2(0.0, 0.0),
        m2(0.0, 0.0),
        std(0.0, 0.0),
        std_rv(0.0, 0.0),
        min(INFINITY, INFINITY),
        max(-INFINITY, -INFINITY),
        count(0),
        is_complex(is_complex),
        ignore_zero(ignorezero) {}

  void operator()(complex_type val);

  template <class ImageType> void print(ImageType &ima, const std::vector<std::string> &fields) {

    if (count > 1) {
      std = complex_type(sqrt(m2.real() / value_type(count - 1)), sqrt(m2.imag() / value_type(count - 1)));
      std_rv = complex_type(sqrt((m2.real() + m2.imag()) / value_type(count - 1)));
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
          std::cout << str(mean) << " ";
        else if (fields[n] == "median")
          std::cout << (!values.empty() ? str(Math::median(values)) : "N/A") << " ";
        else if (fields[n] == "std")
          std::cout << (count > 1 ? str(std) : "N/A") << " ";
        else if (fields[n] == "std_rv")
          std::cout << (count > 1 ? str(std_rv) : "N/A") << " ";
        else if (fields[n] == "iqr")
          std::cout << (!values.empty() ? str(Math::quantile(values, 0.75) - Math::quantile(values, 0.25)) : "N/A")
                    << " ";
        else if (fields[n] == "min")
          std::cout << str(min) << " ";
        else if (fields[n] == "max")
          std::cout << str(max) << " ";
        else if (fields[n] == "count")
          std::cout << count << " ";
        else
          throw Exception("stats type not supported: " + fields[n]);
      }
      std::cout << "\n";

    } else {

      std::string s = "[ ";
      if (ima.ndim() > 3)
        for (size_t n = 3; n < ima.ndim(); n++)
          s += str(ima.index(n)) + " ";
      else
        s += "0 ";
      s += "]";

      int width = is_complex ? 20 : 10;
      std::cout << std::setw(12) << std::right << s << " ";

      std::cout << std::setw(width) << std::right << (count ? str(mean) : "N/A");

      if (!is_complex) {
        std::cout << " " << std::setw(width) << std::right;
        if (count)
          std::cout << Math::median(values);
        else
          std::cout << "N/A";
      }
      std::cout << " " << std::setw(width) << std::right << (count > 1 ? str(std) : "N/A") << " " << std::setw(width)
                << std::right << (count ? str(min) : "N/A") << " " << std::setw(width) << std::right
                << (count ? str(max) : "N/A") << " " << std::setw(10) << std::right << count << "\n";
    }
  }

private:
  complex_type mean, delta, delta2, m2, std, std_rv, min, max;
  size_t count;
  const bool is_complex, ignore_zero;
  std::vector<float> values;
};

void print_header(bool is_complex);

} // namespace MR::Stats
