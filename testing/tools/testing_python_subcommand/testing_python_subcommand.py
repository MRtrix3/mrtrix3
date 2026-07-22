# Copyright (c) 2008-2026 the MRtrix3 contributors.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Covered Software is provided under this License on an "as is"
# basis, without warranty of any kind, either expressed, implied, or
# statutory, including, without limitation, warranties that the
# Covered Software is free of defects, merchantable, fit for a
# particular purpose or non-infringing.
# See the Mozilla Public License v. 2.0 for more details.
#
# For more details, see http://www.mrtrix.org/.

# This tool is the multi-sub-command (multi-algorithm) counterpart of testing_python_cli: it
#   exercises the native Python parser's algorithm-dispatch sub-parsers (add_subparsers, first-
#   positional dispatch, per-algorithm -help, main-only export, nested RST section generation) and
#   centralises coverage of every collective option-group constraint class. The two algorithm
#   sub-interfaces "alpha" and "beta" between them host the presence-forcing constraints
#   (require_exactly_one / require_at_least_one / all_or_none), while the two "at most one"
#   constraints (a nested mutually_exclusive sub-group and a cross-group mutual-exclusion set) are
#   carried by the command's common options so that they apply to whichever algorithm is selected.
#   The inherited verbosity sub-group of the standard options provides a second, nested,
#   mutually_exclusive instance.

import importlib

def usage(cmdline): #pylint: disable=unused-variable
  cmdline.set_author('Robert E. Smith (robert.smith@florey.edu.au)')
  cmdline.set_synopsis('Verify operation of the Python hierarchical (multi-sub-command) command-line interface')
  cmdline.add_description('The algorithm to be used is nominated as the first argument;'
                          ' the subsequent arguments and options available depend on the nominated algorithm.')

  # Common options, shared across every algorithm. The parent group holds a direct option and a
  #   nested child sub-group whose two flags are mutually exclusive (the nested-group form into
  #   which subset mutual-exclusions are expressed).
  common = cmdline.add_argument_group('Grouped common options demonstrating hierarchy')
  common.add_argument('-mode_common',
                      action='store_true',
                      default=None,
                      help='An option located directly within the common parent group')
  mutex = common.add_subgroup('Mutually exclusive common modes')
  mutex.add_argument('-mutex_a',
                     action='store_true',
                     default=None,
                     help='The first mutually-exclusive common mode')
  mutex.add_argument('-mutex_b',
                     action='store_true',
                     default=None,
                     help='The second mutually-exclusive common mode')
  mutex.mutually_exclusive()

  # Two groups each contributing one member to a cross-group mutual-exclusion set: the excluded
  #   options are only a subset of their respective groups, so the exclusion is declared as a
  #   command-level cross-group set rather than a group-level constraint.
  cross_first = cmdline.add_argument_group('Cross-group set, first member')
  cross_first.add_argument('-cross_a',
                           action='store_true',
                           default=None,
                           help='The first member of the cross-group mutual-exclusion set')
  cross_second = cmdline.add_argument_group('Cross-group set, second member')
  cross_second.add_argument('-cross_b',
                            action='store_true',
                            default=None,
                            help='The second member of the cross-group mutual-exclusion set')
  cmdline.flag_mutually_exclusive_options(['cross_a', 'cross_b'])

  # Register the per-algorithm sub-parsers (alpha, beta) declared in this package.
  cmdline.add_subparsers()



def execute(): #pylint: disable=unused-variable
  from mrtrix3 import app #pylint: disable=no-name-in-module, import-outside-toplevel, import-error
  algorithm_module = importlib.import_module(f'.{app.ARGS.algorithm}', 'mrtrix3.commands.testing_python_subcommand')
  algorithm_module.execute()
