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

#include "gtest/gtest.h"

#include "exception.h"
#include "formats/mrtrix_utils.h"
#include "mrtrix.h"

#include <array>
#include <cstdint>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

using namespace MR;

struct ParseAxesParam {
  size_t ndim;
  std::string input_str;
  std::vector<ssize_t> expected_values;
  enum class ExceptionPolicy : uint8_t { Expected, NotExpected };
  ExceptionPolicy exception_policy = ExceptionPolicy::NotExpected;

  friend std::ostream &operator<<(std::ostream &os, const ParseAxesParam &p) {
    os << "InputStr: \"" << p.input_str << "\", ExpectedValues: " << MR::str(p.expected_values);
    return os;
  }
};

class ParseAxesTest : public ::testing::Test {};

const std::array<ParseAxesParam, 13> parse_axes_test_cases{
    {{3, "0,1,2", {1, 2, 3}},
     {3, "+0,+1,+2", {1, 2, 3}},
     {3, "-0,-1,-2", {-1, -2, -3}},
     {3, "0,1,2,", {1, 2, 3}}, // trailing comma tolerated
     {3, "0,1,3", {}, ParseAxesParam::ExceptionPolicy::Expected},
     {2, "0,1,2", {}, ParseAxesParam::ExceptionPolicy::Expected},
     {4, "0,1,2,", {}, ParseAxesParam::ExceptionPolicy::Expected},
     {3, ",0,1,2,", {}, ParseAxesParam::ExceptionPolicy::Expected},
     {4, ",0,1,2,", {}, ParseAxesParam::ExceptionPolicy::Expected},
     {3, "0,1,1", {}, ParseAxesParam::ExceptionPolicy::Expected},
     {3, "0,1,-1", {}, ParseAxesParam::ExceptionPolicy::Expected},
     {3, "0,1,a", {}, ParseAxesParam::ExceptionPolicy::Expected},
     {3, "0,1,2a", {}, ParseAxesParam::ExceptionPolicy::Expected}}};

TEST_F(ParseAxesTest, HandlesVariousFormats) {

  auto test = [](std::string_view input, const ParseAxesParam &param) -> void {
    std::vector<ssize_t> actual_values;
    if (param.exception_policy == ParseAxesParam::ExceptionPolicy::Expected) {
      EXPECT_THROW(actual_values = MR::Formats::parse_axes(param.ndim, input), MR::Exception)
          << "Input string: \"" << input << "\" with " << param.ndim << " dimensions should throw an exception.";
    } else {
      EXPECT_NO_THROW(actual_values = MR::Formats::parse_axes(param.ndim, input))
          << "Input string: \"" << input << "\" with " << param.ndim << " dimensions should not throw an exception.";
    }
    EXPECT_EQ(actual_values, param.expected_values) << "Input string: \"" << input << "\""
                                                    << " with " << param.ndim << " dimensions\n"
                                                    << "  Expected: " << MR::str(param.expected_values) << "\n"
                                                    << "  Actual:   " << MR::str(actual_values);
  };

  for (const auto &param : parse_axes_test_cases) {
    {
      const std::string_view input(param.input_str);
      test(input, param);
    }
    {
      char *const array_not_null_terminated = new char[param.input_str.size()];
      memcpy(array_not_null_terminated, param.input_str.c_str(), param.input_str.size());
      const std::string_view input(array_not_null_terminated, param.input_str.size());
      test(input, param);
      delete[] array_not_null_terminated;
    }
  }
}
