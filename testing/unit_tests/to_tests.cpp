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
#include "mrtrix.h"

#include <complex>
#include <cstddef>
#include <cstdint>
#include <fmt/format.h>
#include <string>
#include <vector>

using namespace MR;

namespace {
const std::vector<std::string> kInputStrings = {
    "0",
    "1",
    "2",
    "0 ",
    " 1",
    "0 0",
    "0a",
    "a0",
    "true",
    "TRUE",
    "tru",
    "truee",
    "false",
    "FALSE",
    "fals",
    "falsee",
    "true ",
    "yes",
    "YES",
    "yeah",
    "yess",
    "no",
    "NO",
    "nope",
    "na",
    "0.0",
    "1e",
    "1e-1",
    "1e-1a",
    "inf",
    "INF",
    "infinity",
    "-inf",
    "-infinity",
    "nan",
    "NAN",
    "nana",
    "-nan",
    "i",
    "I",
    "j",
    "J",
    "-i",
    "1i",
    "1i0",
    "1+i",
    "1+ii",
    "a1+i",
    "1+1+i",
    "-1-i",
    "inf+infi",
    " -inf+-nani ",
    // fmtlib's parenthesised complex form "(a+bi)" and malformed parenthesised inputs
    "(1+2i)",
    "(0+0i)",
    "(-1.5-2.5i)",
    "(3+0i)",
    "(1+ii)",
    "()"};

const std::vector<bool> expectedBoolResults = {
    true,  // "0"
    true,  // "1"
    true,  // "2"
    true,  // "0 "
    true,  // " 1"
    false, // "0 0"
    false, // "0a"
    false, // "a0"
    true,  // "true"
    true,  // "TRUE"
    false, // "tru"
    false, // "truee"
    true,  // "false"
    true,  // "FALSE"
    false, // "fals"
    false, // "falsee"
    true,  // "true "
    true,  // "yes"
    true,  // "YES"
    false, // "yeah"
    false, // "yess"
    true,  // "no"
    true,  // "NO"
    false, // "nope"
    false, // "na"
    false, // "0.0"
    false, // "1e"
    false, // "1e-1"
    false, // "1e-1a"
    false, // "inf"
    false, // "INF"
    false, // "infinity"
    false, // "-inf"
    false, // "-infinity"
    false, // "nan"
    false, // "NAN"
    false, // "nana"
    false, // "-nan"
    false, // "i"
    false, // "I"
    false, // "j"
    false, // "J"
    false, // "-i"
    false, // "1i"
    false, // "1i0"
    false, // "1+i"
    false, // "1+ii"
    false, // "a1+i"
    false, // "1+1+i"
    false, // "-1-i"
    false, // "inf+infi"
    false, // " -inf+-nani "
    false, // "(1+2i)"
    false, // "(0+0i)"
    false, // "(-1.5-2.5i)"
    false, // "(3+0i)"
    false, // "(1+ii)"
    false  // "()"
};

const std::vector<bool> expectedIntResults = {
    true,  // "0"
    true,  // "1"
    true,  // "2"
    true,  // "0 "
    true,  // " 1"
    false, // "0 0"
    false, // "0a"
    false, // "a0"
    false, // "true"
    false, // "TRUE"
    false, // "tru"
    false, // "truee"
    false, // "false"
    false, // "FALSE"
    false, // "fals"
    false, // "falsee"
    false, // "true "
    false, // "yes"
    false, // "YES"
    false, // "yeah"
    false, // "yess"
    false, // "no"
    false, // "NO"
    false, // "nope"
    false, // "na"
    false, // "0.0"
    false, // "1e"
    false, // "1e-1"
    false, // "1e-1a"
    false, // "inf"
    false, // "INF"
    false, // "infinity"
    false, // "-inf"
    false, // "-infinity"
    false, // "nan"
    false, // "NAN"
    false, // "nana"
    false, // "-nan"
    false, // "i"
    false, // "I"
    false, // "j"
    false, // "J"
    false, // "-i"
    false, // "1i"
    false, // "1i0"
    false, // "1+i"
    false, // "1+ii"
    false, // "a1+i"
    false, // "1+1+i"
    false, // "-1-i"
    false, // "inf+infi"
    false, // " -inf+-nani "
    false, // "(1+2i)"
    false, // "(0+0i)"
    false, // "(-1.5-2.5i)"
    false, // "(3+0i)"
    false, // "(1+ii)"
    false  // "()"
};

// Note: the expected results below include "infinity"/"-infinity" as true, whereas the
// historical std::istringstream-based implementation rejected them (istream consumed "inf"
// then failed the trailing "inity"). std::from_chars accepts the full "infinity" keyword
// per the C++17 specification, and that more standard-conformant behaviour is now reflected
// here. The short forms "inf"/"-inf" remain accepted, as do "nan"/"-nan".
const std::vector<bool> expectedFloatResults = {
    true,  // "0"
    true,  // "1"
    true,  // "2"
    true,  // "0 "
    true,  // " 1"
    false, // "0 0"
    false, // "0a"
    false, // "a0"
    false, // "true"
    false, // "TRUE"
    false, // "tru"
    false, // "truee"
    false, // "false"
    false, // "FALSE"
    false, // "fals"
    false, // "falsee"
    false, // "true "
    false, // "yes"
    false, // "YES"
    false, // "yeah"
    false, // "yess"
    false, // "no"
    false, // "NO"
    false, // "nope"
    false, // "na"
    true,  // "0.0"
    false, // "1e"
    true,  // "1e-1"
    false, // "1e-1a"
    true,  // "inf"
    true,  // "INF"
    true,  // "infinity"
    true,  // "-inf"
    true,  // "-infinity"
    true,  // "nan"
    true,  // "NAN"
    false, // "nana"
    true,  // "-nan"
    false, // "i"
    false, // "I"
    false, // "j"
    false, // "J"
    false, // "-i"
    false, // "1i"
    false, // "1i0"
    false, // "1+i"
    false, // "1+ii"
    false, // "a1+i"
    false, // "1+1+i"
    false, // "-1-i"
    false, // "inf+infi"
    false, // " -inf+-nani "
    false, // "(1+2i)"
    false, // "(0+0i)"
    false, // "(-1.5-2.5i)"
    false, // "(3+0i)"
    false, // "(1+ii)"
    false  // "()"
};

const std::vector<bool> expectedComplexResults = {
    true,  // "0"
    true,  // "1"
    true,  // "2"
    true,  // "0 "
    true,  // " 1"
    false, // "0 0"
    false, // "0a"
    false, // "a0"
    false, // "true"
    false, // "TRUE"
    false, // "tru"
    false, // "truee"
    false, // "false"
    false, // "FALSE"
    false, // "fals"
    false, // "falsee"
    false, // "true "
    false, // "yes"
    false, // "YES"
    false, // "yeah"
    false, // "yess"
    false, // "no"
    false, // "NO"
    false, // "nope"
    false, // "na"
    true,  // "0.0"
    false, // "1e"
    true,  // "1e-1"
    false, // "1e-1a"
    true,  // "inf"
    true,  // "INF"
    true,  // "infinity"
    true,  // "-inf"
    true,  // "-infinity"
    true,  // "nan"
    true,  // "NAN"
    false, // "nana"
    true,  // "-nan"
    true,  // "i"
    false, // "I"
    true,  // "j"
    false, // "J"
    true,  // "-i"
    true,  // "1i"
    false, // "1i0"
    true,  // "1+i"
    false, // "1+ii"
    false, // "a1+i"
    false, // "1+1+i"
    true,  // "-1-i"
    true,  // "inf+infi"
    true,  //" -inf+-nani "
    true,  // "(1+2i)"
    true,  // "(0+0i)"
    true,  // "(-1.5-2.5i)"
    true,  // "(3+0i)"
    false, // "(1+ii)"
    false  // "()"
};

} // namespace

class ToBoolTest : public ::testing::Test {};

TEST_F(ToBoolTest, StringToBoolConversion) {
  for (size_t i = 0; i < kInputStrings.size(); ++i) {
    std::string_view input_string = kInputStrings[i];
    const bool expect_success = expectedBoolResults[i];

    if (expect_success) {
      EXPECT_NO_THROW(MR::to<bool>(input_string)) << "Input: \"" << input_string << "\" to bool should succeed.";
    } else {
      EXPECT_THROW(MR::to<bool>(input_string), MR::Exception)
          << "Input: \"" << input_string << "\" to bool should fail.";
    }
  }
}

class ToIntTest : public ::testing::Test {};

TEST_F(ToIntTest, StringToIntConversion) {
  for (size_t i = 0; i < kInputStrings.size(); ++i) {
    std::string_view input_string = kInputStrings[i];
    const bool expect_success = expectedIntResults[i];

    if (expect_success) {
      EXPECT_NO_THROW(MR::to<int>(input_string)) << "Input: \"" << input_string << "\" to int should succeed.";
    } else {
      EXPECT_THROW(MR::to<int>(input_string), MR::Exception) << "Input: \"" << input_string << "\" to int should fail.";
    }
  }
}

class ToFloatTest : public ::testing::Test {};

TEST_F(ToFloatTest, StringToFloatConversion) {
  for (size_t i = 0; i < kInputStrings.size(); ++i) {
    std::string_view input_string = kInputStrings[i];
    const bool expect_success = expectedFloatResults[i];

    if (expect_success) {
      EXPECT_NO_THROW(MR::to<float>(input_string)) << "Input: \"" << input_string << "\" to float should succeed.";
    } else {
      EXPECT_THROW(MR::to<float>(input_string), MR::Exception)
          << "Input: \"" << input_string << "\" to float should fail.";
    }
  }
}

class ToComplexFloatTest : public ::testing::Test {};

TEST_F(ToComplexFloatTest, StringToComplexFloatConversion) {
  for (size_t i = 0; i < kInputStrings.size(); ++i) {
    std::string_view input_string = kInputStrings[i];
    const bool expect_success = expectedComplexResults[i];

    if (expect_success) {
      EXPECT_NO_THROW(MR::to<std::complex<float>>(input_string))
          << "Input: \"" << input_string << "\" to std::complex<float> should succeed.";
    } else {
      EXPECT_THROW(MR::to<std::complex<float>>(input_string), MR::Exception)
          << "Input: \"" << input_string << "\" to std::complex<float> should fail.";
    }
  }
}

// fmtlib formats complex values as "(a+bi)"; verify the parenthesised form round-trips back
// through MR::to<>() to the same value as the equivalent bare "a+bi" form.
TEST_F(ToComplexFloatTest, ParenthesisedComplexRoundTrip) {
  EXPECT_EQ(MR::to<std::complex<float>>("(1.5+2.5i)"), std::complex<float>(1.5f, 2.5f));
  EXPECT_EQ(MR::to<std::complex<float>>("(1.5+2.5i)"), MR::to<std::complex<float>>("1.5+2.5i"));
  EXPECT_EQ(MR::to<std::complex<float>>("(-1-2i)"), std::complex<float>(-1.0f, -2.0f));
  EXPECT_EQ(MR::to<std::complex<float>>("(3+0i)"), std::complex<float>(3.0f, 0.0f));

  EXPECT_EQ(MR::to<std::complex<double>>("(1.5+2.5i)"), std::complex<double>(1.5, 2.5));
  EXPECT_EQ(MR::to<std::complex<double>>("(1.5+2.5i)"), MR::to<std::complex<double>>("1.5+2.5i"));
  EXPECT_EQ(MR::to<std::complex<double>>("(-1-2i)"), std::complex<double>(-1.0, -2.0));
}

// Drive the complex specialisation directly off fmtlib's complex formatter output rather than
// hard-coded literals, so the test pins the actual fmtlib<->MR::to<>() contract. fmtlib elides
// a zero real/imag component (so {3,0} renders as "3+0i" without the outer brackets, and {0,-7.25}
// as "-7.25i"); MR::to<>() must accept both the parenthesised form and any shorter form fmtlib
// chooses to emit.
TEST_F(ToComplexFloatTest, FmtlibFormatRoundTrip) {
  const std::vector<std::complex<float>> cf_values = {
      {0.0f, 0.0f},
      {1.5f, 2.5f},
      {-1.5f, -2.5f},
      {3.0f, 0.0f},
      {0.0f, -7.25f},
  };
  for (const auto &value : cf_values) {
    const std::string formatted = fmt::format("{}", value);
    ASSERT_FALSE(formatted.empty()) << "fmt::format produced empty string";
    EXPECT_EQ(MR::to<std::complex<float>>(formatted), value)
        << "round-trip failed for fmt-formatted complex \"" << formatted << "\"";
  }

  const std::vector<std::complex<double>> cd_values = {
      {0.0, 0.0},
      {1.5, 2.5},
      {-1.5, -2.5},
      {3.0, 0.0},
  };
  for (const auto &value : cd_values) {
    const std::string formatted = fmt::format("{}", value);
    EXPECT_EQ(MR::to<std::complex<double>>(formatted), value)
        << "round-trip failed for fmt-formatted complex \"" << formatted << "\"";
  }
}

// fmtlib's parenthesised form is meaningful only for complex types; real-valued conversions
// must reject the brackets so that a real-valued spec accidentally fed a complex literal
// fails loudly rather than silently truncating to the real component.
TEST(ToRealParenthesesRejection, ParenthesesRejectedForReal) {
  const std::vector<std::string> inputs = {
      "(1)",   // integer
      "(1.0)", // floating
      "(1.5+2.5i)",
      "(-1-2i)",
      "(0)",
      "()",
  };
  for (const auto &input : inputs) {
    EXPECT_THROW(MR::to<int>(input), MR::Exception)
        << "Parenthesised input \"" << input << "\" must be rejected by MR::to<int>()";
    EXPECT_THROW(MR::to<float>(input), MR::Exception)
        << "Parenthesised input \"" << input << "\" must be rejected by MR::to<float>()";
    EXPECT_THROW(MR::to<double>(input), MR::Exception)
        << "Parenthesised input \"" << input << "\" must be rejected by MR::to<double>()";
  }
}

// std::from_chars exposes overflow as std::errc::result_out_of_range; MR::to<>() must surface
// that distinct condition as an Exception rather than silently saturating or wrapping. The
// istringstream fallback yields the same observable behaviour (failbit set, value clamped to
// numeric_limits<T>::max()), so this test pins the user-facing contract independent of which
// backend was selected at configure time.
template <typename T> std::string overflow_above_max() {
  // Multiply numeric_limits::max() by 10 by appending a digit: cheap, portable, and avoids any
  // arithmetic on the integer type itself (which would itself overflow).
  return fmt::format("{}0", std::numeric_limits<T>::max());
}

template <typename T> std::string overflow_below_min() { return fmt::format("{}0", std::numeric_limits<T>::min()); }

TEST(ToOverflow, IntegerOverflowThrows) {
  EXPECT_THROW(MR::to<int8_t>(overflow_above_max<int8_t>()), MR::Exception);
  EXPECT_THROW(MR::to<int8_t>(overflow_below_min<int8_t>()), MR::Exception);
  EXPECT_THROW(MR::to<int16_t>(overflow_above_max<int16_t>()), MR::Exception);
  EXPECT_THROW(MR::to<int16_t>(overflow_below_min<int16_t>()), MR::Exception);
  EXPECT_THROW(MR::to<int32_t>(overflow_above_max<int32_t>()), MR::Exception);
  EXPECT_THROW(MR::to<int32_t>(overflow_below_min<int32_t>()), MR::Exception);
  EXPECT_THROW(MR::to<int64_t>(overflow_above_max<int64_t>()), MR::Exception);
  EXPECT_THROW(MR::to<int64_t>(overflow_below_min<int64_t>()), MR::Exception);

  EXPECT_THROW(MR::to<uint8_t>(overflow_above_max<uint8_t>()), MR::Exception);
  EXPECT_THROW(MR::to<uint16_t>(overflow_above_max<uint16_t>()), MR::Exception);
  EXPECT_THROW(MR::to<uint32_t>(overflow_above_max<uint32_t>()), MR::Exception);
  EXPECT_THROW(MR::to<uint64_t>(overflow_above_max<uint64_t>()), MR::Exception);

  // Boundary: numeric_limits::max() itself must round-trip cleanly.
  EXPECT_EQ(MR::to<int32_t>(fmt::format("{}", std::numeric_limits<int32_t>::max())),
            std::numeric_limits<int32_t>::max());
  EXPECT_EQ(MR::to<int32_t>(fmt::format("{}", std::numeric_limits<int32_t>::min())),
            std::numeric_limits<int32_t>::min());
}

TEST(ToOverflow, FloatOverflowThrows) {
  // 1e400 is well beyond either float or double's representable range.
  EXPECT_THROW(MR::to<float>("1e400"), MR::Exception);
  EXPECT_THROW(MR::to<float>("-1e400"), MR::Exception);
  EXPECT_THROW(MR::to<double>("1e400"), MR::Exception);
  EXPECT_THROW(MR::to<double>("-1e400"), MR::Exception);
  // A value beyond float range but within double range must be accepted as double.
  EXPECT_THROW(MR::to<float>("1e40"), MR::Exception);
  EXPECT_NO_THROW(MR::to<double>("1e40"));
}
