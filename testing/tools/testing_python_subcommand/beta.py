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

from mrtrix3 import app # pylint: disable=import-error

def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('beta', parents=[base_parser])
  parser.set_author('Robert E. Smith (robert.smith@florey.edu.au)')
  parser.set_synopsis('Second sub-interface; demonstrates the require_at_least_one and all_or_none constraints')
  parser.add_argument('-beta_flag',
                      action='store_true',
                      default=None,
                      help='A flag specific to the beta sub-interface')
  at_least_one = parser.add_argument_group('At-least-one options')
  at_least_one.add_argument('-atleast_a',
                            action='store_true',
                            default=None,
                            help='The first at-least-one member')
  at_least_one.add_argument('-atleast_b',
                            action='store_true',
                            default=None,
                            help='The second at-least-one member')
  at_least_one.require_at_least_one()
  all_or_none = parser.add_argument_group('All-or-none options')
  all_or_none.add_argument('-both_a',
                           action='store_true',
                           default=None,
                           help='The first all-or-none member')
  all_or_none.add_argument('-both_b',
                           action='store_true',
                           default=None,
                           help='The second all-or-none member')
  all_or_none.all_or_none()



def execute(): #pylint: disable=unused-variable
  app.console(f'operation: {app.ARGS.algorithm}')
  if app.ARGS.mode_common:
    app.console('-mode_common option present')
  if app.ARGS.mutex_a:
    app.console('-mutex_a option present')
  if app.ARGS.mutex_b:
    app.console('-mutex_b option present')
  if app.ARGS.cross_a:
    app.console('-cross_a option present')
  if app.ARGS.cross_b:
    app.console('-cross_b option present')
  if app.ARGS.beta_flag:
    app.console('-beta_flag option present')
  if app.ARGS.atleast_a:
    app.console('-atleast_a option present')
  if app.ARGS.atleast_b:
    app.console('-atleast_b option present')
  if app.ARGS.both_a:
    app.console('-both_a option present')
  if app.ARGS.both_b:
    app.console('-both_b option present')
