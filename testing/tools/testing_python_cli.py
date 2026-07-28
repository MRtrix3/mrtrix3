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

  builtins = cmdline.add_option_group('Built-in types')
  builtins.add_option('flag',
                      'A binary flag')
  builtins.add_option('string_implicit',
                      'A built-in string, its type declared through the builtin str',
                      type=str)
  builtins.add_option('string_explicit',
                      'A built-in string (explicit)',
                      type=str)
  # The alias mechanism (spelling-design.md, mirrored in the Python parser): the canonical
  #   name "-choice" also answers to the alias "-choose" (both spellings, and any unambiguous
  #   prefix of either, resolve to this one option), and the choice value "One" additionally
  #   accepts the alias spelling "uno" (canonicalised to "One" at parse time). Both aliases are
  #   silent: help and every machine-readable export present only the canonical spellings.
  builtins.add_option('choice',
                      'A selection of choices',
                      choices=CHOICES,
                      default='One').alias('choose').choice_alias('uno', 'One')
  builtins.add_option('int_builtin',
                      'An integer; built-in type',
                      type=int)
  builtins.add_option('float_builtin',
                      'A floating-point; built-in type',
                      type=float)

  # A multi-argument option holds one ArgumentTuple, whose member fields are individually typed
  #   and manifest the option's command-line syntax from their own display ids (their metavar,
  #   else their field name); a described field additionally renders its own listing line.
  complex_types = cmdline.add_option_group('Multi-argument and repeatable options')
  complex_types.add_option('tuple_names',
                           'A two-argument option; its fields are identified by their names',
                           type=app.Parser.ArgumentTuple(app.Parser.Argument('first', type=str),
                                                         app.Parser.Argument('second', type=str)))
  complex_types.add_option('tuple_metavars',
                           'A two-argument option; its fields override their display ids',
                           type=app.Parser.ArgumentTuple(app.Parser.Argument('first', type=str, metavar='metavar_one'),
                                                         app.Parser.Argument('second', type=str, metavar='metavar_two')))
  complex_types.add_option('tuple_described',
                           'A two-argument option; its fields carry their own descriptions and types',
                           type=app.Parser.ArgumentTuple(app.Parser.Argument('index',
                                                                             'the index of the item',
                                                                             type=app.Parser.Int(0)),
                                                         app.Parser.Argument('value',
                                                                             'the value to be assigned',
                                                                             type=app.Parser.Float(0.0, 1.0))))
  complex_types.add_option('metavar_one',
                           'A single-argument option with metavar="metavar"',
                           type=str,
                           metavar='metavar')
  complex_types.add_option('multiple',
                           'A command-line option that may be specified multiple times',
                           type=str,
                           allow_multiple=True)
  complex_types.add_option('unused',
                           'An option deliberately left unread to exercise unused-option tracking')

  custom = cmdline.add_option_group('Custom types')
  custom.add_option('bool',
                    'A boolean input',
                    type=app.Parser.Bool())
  custom.add_option('int_unbound',
                    'An integer; unbounded',
                    type=app.Parser.Int())
  custom.add_option('int_nonnegative',
                    'An integer; non-negative',
                    type=app.Parser.Int(0))
  custom.add_option('int_bounded',
                    'An integer; bound range',
                    type=app.Parser.Int(0, 100),
                    default=50)
  custom.add_option('float_unbound',
                    'A floating-point; unbounded',
                    type=app.Parser.Float())
  custom.add_option('float_nonneg',
                    'A floating-point; non-negative',
                    type=app.Parser.Float(0.0))
  custom.add_option('float_bounded',
                    'A floating-point; bound range',
                    type=app.Parser.Float(0.0, 1.0),
                    default=0.5)
  custom.add_option('int_seq',
                    'A comma-separated list of integers',
                    type=app.Parser.SequenceInt())
  custom.add_option('float_seq',
                    'A comma-separated list of floating-points',
                    type=app.Parser.SequenceFloat())
  # Dedicated spherical-harmonic degree types (mirrors the C++ type_lmax / type_sequence_lmax):
  #   a scalar lmax (non-negative even integer) and a comma-separated vector (each non-negative even).
  custom.add_option('lmax',
                    'A spherical-harmonic degree; non-negative even integer',
                    type=app.Parser.Lmax())
  custom.add_option('lmax_bounded',
                    'A spherical-harmonic degree with an explicit upper bound',
                    type=app.Parser.Lmax(0, 8))
  custom.add_option('lmax_seq',
                    'A comma-separated sequence of spherical-harmonic degrees',
                    type=app.Parser.SequenceLmax())
  custom.add_option('dir_in',
                    'An input directory',
                    type=app.Parser.DirectoryIn())
  custom.add_option('dir_out',
                    'An output directory',
                    type=app.Parser.DirectoryOut())
  custom.add_option('file_in',
                    'An input file',
                    type=app.Parser.FileIn())
  custom.add_option('file_out',
                    'An output file',
                    type=app.Parser.FileOut())
  custom.add_option('image_in',
                    'An input image',
                    type=app.Parser.ImageIn())
  custom.add_option('image_out',
                    'An output image',
                    type=app.Parser.ImageOut())
  custom.add_option('tracks_in',
                    'An input tractogram',
                    type=app.Parser.TracksIn())
  custom.add_option('tracks_out',
                    'An output tractogram',
                    type=app.Parser.TracksOut())
  custom.add_option('custom',
                    'An option with custom type',
                    type=Custom())



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
