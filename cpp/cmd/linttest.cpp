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

// This command is a CI canary: it intentionally uses every coding-style
// pattern that check_syntax and clang-tidy-enforce are meant to reject.
// If CI passes on this file, enforcement is broken.

#include <cerrno>
#include <cstdlib>
#include <cstring>

#include "command.h"

// check_syntax: #define for numeric constants (use constexpr)
#define LINTTEST_SCALE 2.0
#define LINTTEST_NRESULTS 8
#define LINTTEST_DEFAULT_LABEL "unlabelled"

// check_syntax: using namespace std
using namespace MR;
using namespace App;
using namespace std;

// check_syntax: sequentially-defined (non-nested) namespaces
namespace MR {
namespace LintTest {

// clang-tidy bugprone-reserved-identifier: double-underscore prefix is reserved
class __ValueAccumulator {
public:
  explicit __ValueAccumulator(int n) : capacity_(n) {}

  void add(double v) {
    if (count_ < capacity_)
      buf_[count_++] = v;
  }

  double mean() const {
    if (count_ == 0)
      return 0.0;
    double s = 0.0;
    for (int i = 0; i < count_; ++i)
      s += buf_[i];
    return s / count_;
  }

private:
  // check_syntax: C-style array member (use std::array<>)
  double buf_[8];
  int capacity_;
  int count_ = 0;
};

// clang-tidy bugprone-macro-parentheses: argument not parenthesised
#define LINTTEST_DOUBLE(x) x * 2.0

} // namespace LintTest
} // namespace MR

// check_syntax: const std::string& parameter (prefer std::string_view)
std::string format_result(const std::string &label, double value) { return label + " = " + str(value); }

// clang-format off
void usage() {

  AUTHOR = "CI Test (ci@mrtrix.org)";

  SYNOPSIS = "Lint-enforcement CI canary command";

  DESCRIPTION
    + "This command deliberately violates every coding-standard rule that"
      " check_syntax and the clang-tidy-enforce CI workflow are intended to"
      " catch, while still compiling cleanly."
    + "If CI passes on this file, lint enforcement is not working correctly.";

  ARGUMENTS
    + Argument("value", "a numeric value to process").type_float();

  OPTIONS
    + Option("label", "label for the output line")
      + Argument("name").type_text();

}
// clang-format on

void run() {
  // check_syntax: C-style cast (use static_cast<>)
  int iterations = (int)LINTTEST_SCALE;

  // check_syntax: C-style array local variable (use std::array<> or std::vector<>)
  double results[LINTTEST_NRESULTS];

  // check_syntax: srand()/rand() — not thread-safe; use <random> instead
  // clang-tidy concurrency-mt-unsafe
  srand(42);

  // check_syntax: getenv() — not thread-safe; use MR::get_env() instead
  // clang-tidy concurrency-mt-unsafe
  // check_syntax: char* (C-style string pointer)
  const char *home_dir = getenv("HOME");

  // check_syntax: strerror() — not thread-safe; use MR::C_strerror() instead
  // clang-tidy concurrency-mt-unsafe
  // check_syntax: char* (C-style string pointer)
  const char *err_msg = strerror(errno);

  // check_syntax: NULL macro (use nullptr)
  const char *label_cstr = NULL;
  if (label_cstr == NULL)
    label_cstr = LINTTEST_DEFAULT_LABEL;

  // check_syntax: NAN macro (use std::numeric_limits<double>::quiet_NaN())
  double sentinel = NAN;

  // check_syntax: std::abs() (use std::fabs() for scalars or MR::abs() for generic)
  double input = argument[0];
  double magnitude = std::abs(input);

  MR::LintTest::__ValueAccumulator acc(iterations);
  for (int i = 0; i < iterations; ++i) {
    // check_syntax: rand() repeated + C-style cast
    results[i] = (double)rand() * magnitude * LINTTEST_DOUBLE(1.0);
    acc.add(results[i]);
  }

  auto opt = get_options("label");
  std::string label = opt.empty() ? std::string(label_cstr) : std::string(opt[0][0]);

  if (home_dir != NULL)
    CONSOLE("Home: " + std::string(home_dir));

  CONSOLE(std::string(err_msg));
  CONSOLE(format_result(label, acc.mean()));
  CONSOLE(str(sentinel));
}
