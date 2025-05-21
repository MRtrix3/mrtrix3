#!/usr/bin/python3

# Copyright (c) 2008-2024 the MRtrix3 contributors.
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

CHOICES = ('One', 'Two', 'Three')

def usage(cmdline): #pylint: disable=unused-variable
  from mrtrix3 import app #pylint: disable=no-name-in-module, import-outside-toplevel

  cmdline.set_author('Robert E. Smith (robert.smith@florey.edu.au)')
  cmdline.set_synopsis('Test operation of the Python command-line interface')

  builtins = cmdline.add_argument_group('Built-in types')
  builtins.add_argument('-flag',
                        action='store_true',
                        default=None,
                        help='A binary flag')
  builtins.add_argument('-string_implicit',
                        help='A built-in string (implicit)')
  builtins.add_argument('-string_explicit',
                        type=str,
                        help='A built-in string (explicit)')
  builtins.add_argument('-choice',
                        choices=CHOICES,
                        help='A selection of choices; one of: ' + ', '.join(CHOICES))
  builtins.add_argument('-int_builtin',
                        type=int,
                        help='An integer; built-in type')
  builtins.add_argument('-float_builtin',
                        type=float,
                        help='A floating-point; built-in type')

  complex = cmdline.add_argument_group('Complex interfaces; nargs, metavar, etc.')
  complex.add_argument('-nargs_plus',
                       nargs='+',
                       help='A command-line option with nargs="+", no metavar')
  complex.add_argument('-nargs_asterisk',
                       nargs='*',
                       help='A command-line option with nargs="*", no metavar')
  complex.add_argument('-nargs_question',
                       nargs='?',
                       help='A command-line option with nargs="?", no metavar')
  complex.add_argument('-nargs_two',
                       nargs=2,
                       help='A command-line option with nargs=2, no metavar')
  complex.add_argument('-metavar_one',
                       metavar='metavar',
                       help='A command-line option with nargs=1 and metavar="metavar"')
  complex.add_argument('-metavar_two',
                       metavar='metavar',
                       nargs=2,
                       help='A command-line option with nargs=2 and metavar="metavar"')
  complex.add_argument('-metavar_tuple',
                       metavar=('metavar_one', 'metavar_two'),
                       nargs=2,
                       help='A command-line option with nargs=2 and metavar=("metavar_one", "metavar_two")')
  complex.add_argument('-append',
                       action='append',
                       help='A command-line option with "append" action (can be specified multiple times)')

  custom = cmdline.add_argument_group('Custom types')
  custom.add_argument('-bool',
                      type=app.Parser.Bool(),
                      help='A boolean input')
  custom.add_argument('-int_unbound',
                      type=app.Parser.Int(),
                      help='An integer; unbounded')
  custom.add_argument('-int_nonnegative',
                      type=app.Parser.Int(0),
                      help='An integer; non-negative')
  custom.add_argument('-int_bounded',
                      type=app.Parser.Int(0, 100),
                      help='An integer; bound range')
  custom.add_argument('-float_unbound',
                      type=app.Parser.Float(),
                      help='A floating-point; unbounded')
  custom.add_argument('-float_nonneg',
                      type=app.Parser.Float(0.0),
                      help='A floating-point; non-negative')
  custom.add_argument('-float_bounded',
                      type=app.Parser.Float(0.0, 1.0),
                      help='A floating-point; bound range')
  custom.add_argument('-int_seq',
                      type=app.Parser.SequenceInt(),
                      help='A comma-separated list of integers')
  custom.add_argument('-float_seq',
                      type=app.Parser.SequenceFloat(),
                      help='A comma-separated list of floating-points')
  custom.add_argument('-dir_in',
                      type=app.Parser.DirectoryIn(),
                      help='An input directory')
  custom.add_argument('-dir_out',
                      type=app.Parser.DirectoryOut(),
                      help='An output directory')
  custom.add_argument('-file_in',
                      type=app.Parser.FileIn(),
                      help='An input file')
  custom.add_argument('-file_out',
                      type=app.Parser.FileOut(),
                      help='An output file')
  custom.add_argument('-image_in',
                      type=app.Parser.ImageIn(),
                      help='An input image')
  custom.add_argument('-image_out',
                      type=app.Parser.ImageOut(),
                      help='An output image')
  custom.add_argument('-tracks_in',
                      type=app.Parser.TracksIn(),
                      help='An input tractogram')
  custom.add_argument('-tracks_out',
                      type=app.Parser.TracksOut(),
                      help='An output tractogram')
  custom.add_argument('-various',
                      type=app.Parser.Various(),
                      help='An option that accepts various types of content')



def execute(): #pylint: disable=unused-variable
  from mrtrix3 import app #pylint: disable=no-name-in-module, import-outside-toplevel

  for key in vars(app.ARGS):
    value = getattr(app.ARGS, key)
    if value is not None:
      app.console(f'{key}: {repr(value)}')
