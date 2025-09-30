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

#include <complex>
#include <cstddef>
#include <string>
#include <vector>

using namespace MR;

namespace {
const std::vector<std::string> kInputStrings = {
    "0",         "1",     "2",     "0 ",   " 1",     "0 0",   "0a",       "a0",          "true", "TRUE",     "tru",
    "truee",     "false", "FALSE", "fals", "falsee", "true ", "yes",      "YES",         "yeah", "yess",     "no",
    "NO",        "nope",  "na",    "0.0",  "1e",     "1e-1",  "1e-1a",    "inf",         "INF",  "infinity", "-inf",
    "-infinity", "nan",   "NAN",   "nana", "-nan",   "i",     "I",        "j",           "J",    "-i",       "1i",
    "1i0",       "1+i",   "1+ii",  "a1+i", "1+1+i",  "-1-i",  "inf+infi", " -inf+-nani "};

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
    false  // " -inf+-nani "
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
    false  // " -inf+-nani "
};

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
    false, // "infinity"
    true,  // "-inf"
    false, // "-infinity"
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
    false  // " -inf+-nani "
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
    false, // "infinity"
    true,  // "-inf"
    false, // "-infinity"
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
    true   //" -inf+-nani "
};

} // namespace

class ToBoolTest : public ::testing::Test {};

TEST_F(ToBoolTest, StringToBoolConversion) {
  for (size_t i = 0; i < kInputStrings.size(); ++i) {
    const std::string &input_string = kInputStrings[i];
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
    const std::string &input_string = kInputStrings[i];
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
    const std::string &input_string = kInputStrings[i];
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
    const std::string &input_string = kInputStrings[i];
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
