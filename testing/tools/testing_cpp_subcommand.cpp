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

#include "command.h"

using namespace MR;
using namespace App;

// This tool is the multi-sub-command counterpart of testing_cpp_cli: it exercises the hierarchical
//   command machinery (SUBCOMMANDS dispatch, per-sub-command -help, main-only export, nested RST
//   subdirectory generation) and centralises coverage of every collective option-group constraint
//   class. The two dispatch sub-interfaces "alpha" and "beta" between them host the presence-forcing
//   constraints (require_exactly_one / require_at_least_one / all_or_none), while the two "at most
//   one" constraints (a nested mutually_exclusive sub-group and a cross-group MUTUALLY_EXCLUSIVE_OPTIONS
//   set) are carried by the command's common options so that they apply to whichever sub-interface is
//   selected. The inherited verbosity sub-group of the standard options provides a second, nested,
//   mutually_exclusive instance.

// clang-format off
void usage() {

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Verify operation of the C++ hierarchical (multi-sub-command) command-line interface";

  DESCRIPTION
  + "The operation to be performed is nominated as the first argument;"
    " the subsequent arguments and options available depend on the nominated operation.";

  // Two groups each contributing one member to a cross-group mutual-exclusion set: the excluded
  //   options are only a subset of their respective groups, so the exclusion is declared as a
  //   command-level MUTUALLY_EXCLUSIVE_OPTIONS set rather than a group-level constraint.
  MUTUALLY_EXCLUSIVE_OPTIONS = {{"cross_a", "cross_b"}};

  // Common options, shared across every sub-interface. The parent group holds a direct option and a
  //   nested child sub-group whose two flags are mutually exclusive (the nested-group form into which
  //   subset mutual-exclusions are expressed).
  OPTIONS
  + (OptionGroup("Grouped common options demonstrating hierarchy")
     + Option("mode_common", "An option located directly within the common parent group")
     + (OptionGroup("Mutually exclusive common modes")
        + Option("mutex_a", "The first mutually-exclusive common mode")
        + Option("mutex_b", "The second mutually-exclusive common mode")).mutually_exclusive())

  + (OptionGroup("Cross-group set, first member")
     + Option("cross_a", "The first member of the cross-group mutual-exclusion set"))

  + (OptionGroup("Cross-group set, second member")
     + Option("cross_b", "The second member of the cross-group mutual-exclusion set"));

  SUBCOMMANDS_SELECTOR = "operation";

  SUBCOMMANDS
  + Subcommand("alpha")
      .set_synopsis("First sub-interface; demonstrates the require_exactly_one constraint")
      .set_options(OptionList()
        + Option("alpha_value", "An option specific to the alpha sub-interface")
          + Argument("spec").type_text()
        + (OptionGroup("Exactly-one options")
           + Option("exone_a", "The first exactly-one member")
           + Option("exone_b", "The second exactly-one member")).require_exactly_one())

  + Subcommand("beta")
      .set_synopsis("Second sub-interface; demonstrates the require_at_least_one and all_or_none constraints")
      .set_options(OptionList()
        + Option("beta_flag", "A flag specific to the beta sub-interface")
        + (OptionGroup("At-least-one options")
           + Option("atleast_a", "The first at-least-one member")
           + Option("atleast_b", "The second at-least-one member")).require_at_least_one()
        + (OptionGroup("All-or-none options")
           + Option("both_a", "The first all-or-none member")
           + Option("both_b", "The second all-or-none member")).all_or_none());

}
// clang-format on

void run() {

  const std::string operation = get_subcommand();
  CONSOLE("operation: " + operation);

  // Common options are registered irrespective of the selected sub-interface, so they may always
  //   be queried.
  if (!get_options("mode_common").empty())
    CONSOLE("-mode_common option present");
  if (!get_options("mutex_a").empty())
    CONSOLE("-mutex_a option present");
  if (!get_options("mutex_b").empty())
    CONSOLE("-mutex_b option present");
  if (!get_options("cross_a").empty())
    CONSOLE("-cross_a option present");
  if (!get_options("cross_b").empty())
    CONSOLE("-cross_b option present");

  // Sub-interface-specific options are only registered when their sub-interface is selected; the
  //   registered-option-access invariant forbids querying the other sub-interface's options here.
  if (operation == "alpha") {
    auto opt = get_options("alpha_value");
    if (!opt.empty())
      CONSOLE("-alpha_value: " + std::string(opt[0][0]));
    if (!get_options("exone_a").empty())
      CONSOLE("-exone_a option present");
    if (!get_options("exone_b").empty())
      CONSOLE("-exone_b option present");
  } else if (operation == "beta") {
    if (!get_options("beta_flag").empty())
      CONSOLE("-beta_flag option present");
    if (!get_options("atleast_a").empty())
      CONSOLE("-atleast_a option present");
    if (!get_options("atleast_b").empty())
      CONSOLE("-atleast_b option present");
    if (!get_options("both_a").empty())
      CONSOLE("-both_a option present");
    if (!get_options("both_b").empty())
      CONSOLE("-both_b option present");
  }
}
