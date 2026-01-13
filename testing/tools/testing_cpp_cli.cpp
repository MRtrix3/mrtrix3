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

#include <string>
#include <vector>

#include "command.h"

using namespace MR;
using namespace App;

const std::vector<std::string> choices = {"One", "Two", "Three"};

// clang-format off
void usage() {

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Verify operation of the C++ command-line interface & parser";

  REQUIRES_AT_LEAST_ONE_ARGUMENT = false;

  OPTIONS
  + Option("flag", "An option flag that takes no arguments")

  + Option("text", "a text input")
    + Argument("spec").type_text()

  + Option("bool", "a boolean input")
    + Argument("value").type_bool()

  + Option("int_unbound", "an integer input (unbounded)")
    + Argument("value").type_integer()

  + Option("int_nonneg", "a non-negative integer")
    + Argument("value").type_integer(0)

  + Option("int_bound", "a bound integer")
    + Argument("value").type_integer(0, 100)

  + Option("float_unbound", "a floating-point number (unbounded)")
    + Argument("value").type_float()

  + Option("float_nonneg", "a non-negative floating-point number")
    + Argument("value").type_float(0.0)

  + Option("float_bound", "a bound floating-point number")
    + Argument("value").type_float(0.0, 1.0)

  + Option("int_seq", "a comma-separated sequence of integers")
    + Argument("values").type_sequence_int()

  + Option("float_seq", "a comma-separated sequence of floating-point numbers")
    + Argument("values").type_sequence_float()

  + Option("choice", "a choice from a set of options")
    + Argument("item").type_choice(choices)

  + Option("file_in", "an input file")
    + Argument("input").type_file_in()

  + Option("file_out", "an output file")
    + Argument("output").type_file_out()

  + Option("dir_in", "an input directory")
    + Argument("input").type_directory_in()

  + Option("dir_out", "an output directory")
    + Argument("output").type_directory_out()

  + Option("tracks_in", "an input tractogram")
    + Argument("input").type_tracks_in()

  + Option("tracks_out", "an output tractogram")
    + Argument("output").type_tracks_out()

  + Option("any", "an argument that could accept any of the various forms")
    + Argument("spec").type_text()
                      .type_bool()
                      .type_integer()
                      .type_float()
                      .type_sequence_int()
                      .type_sequence_float()
                      .type_choice(choices)
                      .type_file_in()
                      .type_file_out()
                      .type_directory_in()
                      .type_directory_out()
                      .type_tracks_in()
                      .type_tracks_out()

  + Option("nargs_two", "A command-line option that accepts two arguments")
    + Argument("first").type_text()
    + Argument("second").type_text()

  + Option("multiple", "A command-line option that can be specified multiple times").allow_multiple()
    + Argument("spec").type_text();

}
// clang-format on

void run() {

  if (!get_options("flag").empty())
    CONSOLE("-flag option present");

  auto opt = get_options("text");
  if (!opt.empty())
    CONSOLE("-text: " + std::string(opt[0][0]));
  opt = get_options("bool");
  if (!opt.empty())
    CONSOLE("-bool: " + str(bool(opt[0][0])));
  opt = get_options("int_unbound");
  if (!opt.empty())
    CONSOLE("-int_unbound: " + str(int64_t(opt[0][0])));
  opt = get_options("int_nonneg");
  if (!opt.empty())
    CONSOLE("-int_nonneg: " + str(int64_t(opt[0][0])));
  opt = get_options("int_bound");
  if (!opt.empty())
    CONSOLE("-int_bound: " + str(int64_t(opt[0][0])));
  opt = get_options("float_unbound");
  if (!opt.empty())
    CONSOLE("-float_unbound: " + str(default_type(opt[0][0])));
  opt = get_options("float_nonneg");
  if (!opt.empty())
    CONSOLE("-float_nonneg: " + str(default_type(opt[0][0])));
  opt = get_options("float_bound");
  if (!opt.empty())
    CONSOLE("-float_bound: " + str(default_type(opt[0][0])));
  opt = get_options("int_seq");
  if (!opt.empty())
    CONSOLE("-int_seq: [" + join(parse_ints<int64_t>(opt[0][0]), ",") + "]");
  opt = get_options("float_seq");
  if (!opt.empty())
    CONSOLE("-float_seq: [" + join(parse_floats(opt[0][0]), ",") + "]");
  opt = get_options("choice");
  if (!opt.empty())
    CONSOLE("-choice: " + str(opt[0][0]));
  opt = get_options("file_in");
  if (!opt.empty())
    CONSOLE("-file_in: " + str(opt[0][0]));
  opt = get_options("file_out");
  if (!opt.empty())
    CONSOLE("-file_out: " + str(opt[0][0]));
  opt = get_options("dir_in");
  if (!opt.empty())
    CONSOLE("-dir_in: " + str(opt[0][0]));
  opt = get_options("dir_out");
  if (!opt.empty())
    CONSOLE("-dir_out: " + str(opt[0][0]));
  opt = get_options("tracks_in");
  if (!opt.empty())
    CONSOLE("-tracks_in: " + str(opt[0][0]));
  opt = get_options("tracks_out");
  if (!opt.empty())
    CONSOLE("-tracks_out: " + str(opt[0][0]));

  opt = get_options("any");
  if (!opt.empty())
    CONSOLE("-any: " + str(opt[0][0]));
  opt = get_options("nargs_two");
  if (!opt.empty())
    CONSOLE("-nargs_two: [" + str(opt[0][0]) + " " + str(opt[0][1]) + "]");
  opt = get_options("multiple");
  if (!opt.empty()) {
    std::vector<std::string> specs;
    for (size_t i = 0; i != opt.size(); ++i)
      specs.push_back(std::string("\"") + str(opt[i][0]) + "\"");
    CONSOLE("-multiple: [" + join(specs, " ") + "]");
  }
}
