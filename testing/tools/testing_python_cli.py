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

from mrtrix3 import app # pylint: disable=import-error

CHOICES = ('One', 'Two', 'Three')

class Custom(app.Parser.CustomTypeBase):
  def __call__(self, input_value):
    return input_value
  @staticmethod
  def _legacytypestring():
    return 'CUSTOM'
  @staticmethod
  def _metavar():
    return 'custom'

def usage(cmdline): #pylint: disable=unused-variable
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
  # The alias mechanism (spelling-design.md, mirrored in the Python parser): the canonical
  #   name "-choice" also answers to the alias "-choose" (both spellings, and any unambiguous
  #   prefix of either, resolve to this one option), and the choice value "One" additionally
  #   accepts the alias spelling "uno" (canonicalised to "One" at parse time). Both aliases are
  #   silent: help and every machine-readable export present only the canonical spellings.
  builtins.add_argument('-choice',
                        choices=CHOICES,
                        help='A selection of choices').set_default('One') \
                        .alias('choose').choice_alias('uno', 'One')
  builtins.add_argument('-int_builtin',
                        type=int,
                        help='An integer; built-in type')
  builtins.add_argument('-float_builtin',
                        type=float,
                        help='A floating-point; built-in type')

  complex_types = cmdline.add_argument_group('Complex interfaces; nargs, metavar, etc.')
  complex_types.add_argument('-nargs_two',
                             nargs=2,
                             help='A command-line option with nargs=2, no metavar')
  complex_types.add_argument('-metavar_one',
                             metavar='metavar',
                             help='A command-line option with nargs=1 and metavar="metavar"')
  complex_types.add_argument('-metavar_two',
                             metavar='metavar',
                             nargs=2,
                             help='A command-line option with nargs=2 and metavar="metavar"')
  complex_types.add_argument('-metavar_tuple',
                             metavar=('metavar_one', 'metavar_two'),
                             nargs=2,
                             help='A command-line option with nargs=2 and metavar=("metavar_one", "metavar_two")')
  complex_types.add_argument('-append',
                             action='append',
                             help='A command-line option with "append" action (can be specified multiple times)')
  complex_types.add_argument('-unused',
                             action='store_true',
                             default=None,
                             help='An option deliberately left unread to exercise unused-option tracking')

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
                      help='An integer; bound range').set_default(50)
  custom.add_argument('-float_unbound',
                      type=app.Parser.Float(),
                      help='A floating-point; unbounded')
  custom.add_argument('-float_nonneg',
                      type=app.Parser.Float(0.0),
                      help='A floating-point; non-negative')
  custom.add_argument('-float_bounded',
                      type=app.Parser.Float(0.0, 1.0),
                      help='A floating-point; bound range').set_default(0.5)
  custom.add_argument('-int_seq',
                      type=app.Parser.SequenceInt(),
                      help='A comma-separated list of integers')
  custom.add_argument('-float_seq',
                      type=app.Parser.SequenceFloat(),
                      help='A comma-separated list of floating-points')
  # Dedicated spherical-harmonic degree types (mirrors the C++ type_lmax / type_lmax_sequence):
  #   a scalar lmax (non-negative even integer) and a comma-separated vector (each non-negative even).
  custom.add_argument('-lmax',
                      type=app.Parser.Lmax(),
                      help='A spherical-harmonic degree; non-negative even integer')
  custom.add_argument('-lmax_bounded',
                      type=app.Parser.Lmax(0, 8),
                      help='A spherical-harmonic degree with an explicit upper bound')
  custom.add_argument('-lmax_seq',
                      type=app.Parser.SequenceLmax(),
                      help='A comma-separated sequence of spherical-harmonic degrees')
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
  custom.add_argument('-custom',
                      type=Custom(),
                      help='An option with custom type')



def execute(): #pylint: disable=unused-variable
  for key in vars(app.ARGS):
    # Skip internal parser bookkeeping (e.g. the unused-option access tracker) exposed on the
    #   ARGS namespace under an underscore-prefixed key; only user-facing arguments are reported.
    if key.startswith('_'):
      continue
    # "-unused" is intentionally never read, so that specifying it exercises the end-of-run
    #   unused-option advisory (the command body reading it would mark it as accessed).
    if key == 'unused':
      continue
    value = getattr(app.ARGS, key)
    if value is not None:
      app.console(f'{key}: {repr(value)}')
