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

def usage(base_parser, subcommands): #pylint: disable=unused-variable
  parser = subcommands.add_subcommand('alpha', parent=base_parser)
  parser.set_author('Robert E. Smith (robert.smith@florey.edu.au)')
  parser.set_synopsis('First sub-interface; demonstrates the require_exactly_one constraint')
  parser.add_option('alpha_value',
                    'An option specific to the alpha sub-interface',
                    type=str)
  exactly_one = parser.add_option_group('Exactly-one options')
  exactly_one.add_option('exone_a',
                         'The first exactly-one member')
  exactly_one.add_option('exone_b',
                         'The second exactly-one member')
  exactly_one.require_exactly_one()



def execute(): #pylint: disable=unused-variable
  app.console(f'subcommand: {app.ARGS.subcommand}')
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
  if app.ARGS.alpha_value is not None:
    app.console(f'-alpha_value: {app.ARGS.alpha_value}')
  if app.ARGS.exone_a:
    app.console('-exone_a option present')
  if app.ARGS.exone_b:
    app.console('-exone_b option present')
