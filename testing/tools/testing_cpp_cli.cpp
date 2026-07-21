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

enum class Choice { One, Two, Three };

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
    + Argument("value").type_integer(0, 100).set_default(50)

  + Option("float_unbound", "a floating-point number (unbounded)")
    + Argument("value").type_float()

  + Option("float_nonneg", "a non-negative floating-point number")
    + Argument("value").type_float(0.0)

  + Option("float_bound", "a bound floating-point number")
    + Argument("value").type_float(0.0, 1.0).set_default(0.5)

  + Option("int_seq", "a comma-separated sequence of integers")
    + Argument("values").type_sequence_int()

  + Option("float_seq", "a comma-separated sequence of floating-point numbers")
    + Argument("values").type_sequence_float()

  // The "-choice" option demonstrates auto-rendered help (the choice list and the declared
  //   default), the option alias mechanism (the canonical "-choice" also answers to "-choose"),
  //   and the choice-value alias mechanism (the value "one" additionally accepts "uno").
  + Option("choice", "a choice from a set of options").alias("choose")
    + Argument("item").type_choice<Choice>().choice_alias("uno", "one").set_default("one")

  // Dedicated spherical-harmonic degree types: a scalar lmax (non-negative even integer) and a
  //   comma-separated vector of lmax values (each non-negative and even).
  + Option("lmax", "a spherical-harmonic degree (non-negative even integer)")
    + Argument("value").type_lmax()

  + Option("lmax_bound", "a spherical-harmonic degree with an explicit upper bound")
    + Argument("value").type_lmax(0, 8)

  + Option("lmax_seq", "a comma-separated sequence of spherical-harmonic degrees")
    + Argument("values").type_sequence_lmax()

  + Option("file_in", "an input file")
    + Argument("input").type_file_in()

  + Option("file_out", "an output file")
    + Argument("output").type_file_out()

  + Option("dir_in", "an input directory")
    + Argument("input").type_directory_in()

  + Option("dir_out", "an output directory")
    + Argument("output").type_directory_out(DirOutMode::MayExist)

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
                      .type_choice<Choice>()
                      .type_file_in()
                      .type_file_out()
                      .type_directory_in()
                      .type_directory_out(DirOutMode::MayExist)
                      .type_tracks_in()
                      .type_tracks_out()

  // A multi-field option is a single ArgumentTuple. "-nargs_two" carries no field descriptions,
  //   so it renders exactly as a former multi-scalar-argument option; "-tuple_desc" attaches a
  //   description to each field, exercising the per-field help lines.
  + Option("nargs_two", "A command-line option that accepts two arguments")
    + ArgumentTuple(Argument("first").type_text(),
                    Argument("second").type_text())

  + Option("tuple_desc", "A command-line option whose tuple fields carry descriptions")
    + ArgumentTuple(Argument("key", "the key field").type_text(),
                    Argument("value", "the value field").type_text())

  + Option("multiple", "A command-line option that can be specified multiple times").allow_multiple()
    + Argument("spec").type_text()

  // An option deliberately left unread by run(); specifying it must trigger the end-of-run
  //   "had no effect" advisory of the unused-option tracker.
  + Option("unused", "An option deliberately left unread to exercise unused-option tracking")

  // A hierarchy of option groups: a named parent group holding a direct option and a nested
  //   child sub-group whose two member flags are mutually exclusive (a group constraint).
  + (OptionGroup("Grouped options demonstrating hierarchy")
     + Option("group_direct", "An option located directly within the parent group")
     + (OptionGroup("Mutually exclusive modes")
        + Option("mode_a", "The first mutually-exclusive mode")
        + Option("mode_b", "The second mutually-exclusive mode")).mutually_exclusive());

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
  opt = get_options("lmax");
  if (!opt.empty())
    CONSOLE("-lmax: " + str(int64_t(opt[0][0])));
  opt = get_options("lmax_bound");
  if (!opt.empty())
    CONSOLE("-lmax_bound: " + str(int64_t(opt[0][0])));
  opt = get_options("lmax_seq");
  if (!opt.empty())
    CONSOLE("-lmax_seq: [" + join(opt[0][0].as_sequence_int(), ",") + "]");
  // Filesystem-path argument types must be read with as_text(), never a std::string cast (which
  //   trips the only_filesystem_arg_types() assertion).
  opt = get_options("file_in");
  if (!opt.empty())
    CONSOLE("-file_in: " + opt[0][0].as_text());
  opt = get_options("file_out");
  if (!opt.empty())
    CONSOLE("-file_out: " + opt[0][0].as_text());
  opt = get_options("dir_in");
  if (!opt.empty())
    CONSOLE("-dir_in: " + opt[0][0].as_text());
  opt = get_options("dir_out");
  if (!opt.empty())
    CONSOLE("-dir_out: " + opt[0][0].as_text());
  opt = get_options("tracks_in");
  if (!opt.empty())
    CONSOLE("-tracks_in: " + opt[0][0].as_text());
  opt = get_options("tracks_out");
  if (!opt.empty())
    CONSOLE("-tracks_out: " + opt[0][0].as_text());

  opt = get_options("any");
  if (!opt.empty())
    CONSOLE("-any: " + str(opt[0][0]));
  opt = get_options("nargs_two");
  if (!opt.empty())
    CONSOLE("-nargs_two: [" + str(opt[0][0]) + " " + str(opt[0][1]) + "]");
  opt = get_options("tuple_desc");
  if (!opt.empty())
    CONSOLE("-tuple_desc: [" + str(opt[0]["key"]) + " " + str(opt[0]["value"]) + "]");
  opt = get_options("multiple");
  if (!opt.empty()) {
    std::vector<std::string> specs;
    for (size_t i = 0; i != opt.size(); ++i)
      specs.push_back(std::string("\"") + str(opt[i][0]) + "\"");
    CONSOLE("-multiple: [" + join(specs, " ") + "]");
  }

  if (!get_options("group_direct").empty())
    CONSOLE("-group_direct option present");
  if (!get_options("mode_a").empty())
    CONSOLE("-mode_a option present");
  if (!get_options("mode_b").empty())
    CONSOLE("-mode_b option present");
}
