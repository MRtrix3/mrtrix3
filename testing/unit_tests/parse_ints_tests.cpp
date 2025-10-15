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

#include "gtest/gtest.h"

#include "exception.h"
#include "mrtrix.h"

#include <array>
#include <cstdint>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

using namespace MR;

struct ParseIntsParam {
  std::string input_str;
  std::vector<int> expected_values;
  enum class ExceptionPolicy : uint8_t { Expected, NotExpected };
  ExceptionPolicy exception_policy = ExceptionPolicy::NotExpected;

  friend std::ostream &operator<<(std::ostream &os, const ParseIntsParam &p) {
    os << "InputStr: \"" << p.input_str << "\", ExpectedValues: " << MR::str(p.expected_values);
    return os;
  }
};

class ParseIntsTest : public ::testing::Test {};

const std::array<ParseIntsParam, 20> parse_ints_test_cases{{
    {"1", {1}},
    {"1,3,4", {1, 3, 4}},
    {"5:9", {5, 6, 7, 8, 9}},
    {"2:2:10", {2, 4, 6, 8, 10}},
    {"6:3:-6", {6, 3, 0, -3, -6}},
    {"1:3,5:7", {1, 2, 3, 5, 6, 7}},
    {"1:2:10,20:5:-7", {1, 3, 5, 7, 9, 20, 15, 10, 5, 0, -5}},
    {"1, 5, 7", {1, 5, 7}},
    {"1 5 7", {1, 5, 7}},
    {"1,\t   5\t7", {1, 5, 7}},
    {"1:  5, 7", {1, 2, 3, 4, 5, 7}},
    {"1: 5 7", {1, 2, 3, 4, 5, 7}},
    {"1 :5 7", {1, 2, 3, 4, 5, 7}},
    {"1 : 2 : 5 7", {1, 3, 5, 7}},
    {"1 :2 :-5 7", {1, -1, -3, -5, 7}},
    {"1 : 2: 11 20: 3 :30", {1, 3, 5, 7, 9, 11, 20, 23, 26, 29}},
    {"abc", {}, ParseIntsParam::ExceptionPolicy::Expected},
    {"a,b,c", {}, ParseIntsParam::ExceptionPolicy::Expected},
    {"1,3,c", {}, ParseIntsParam::ExceptionPolicy::Expected},
    {"1:3,c", {}, ParseIntsParam::ExceptionPolicy::Expected},
}};

TEST_F(ParseIntsTest, HandlesVariousFormats) {

  auto test = [](std::string_view input, const ParseIntsParam &param) -> void {
    std::vector<int> actual_values;
    if (param.exception_policy == ParseIntsParam::ExceptionPolicy::Expected) {
      EXPECT_THROW(actual_values = MR::parse_ints<int>(input), MR::Exception)
          << "Input string: \"" << input << "\" should throw an exception.";
    } else {
      EXPECT_NO_THROW(actual_values = MR::parse_ints<int>(input))
          << "Input string: \"" << input << "\" should not throw an exception.";
    }
    EXPECT_EQ(actual_values, param.expected_values)
        << "Input string: \"" << input << "\"\n  Expected: " << MR::str(param.expected_values)
        << "\n  Actual:   " << MR::str(actual_values);
  };

  for (const auto &param : parse_ints_test_cases) {
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
