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


import math, sys



def usage(cmdline): #pylint: disable=unused-variable
  from mrtrix3 import app #pylint: disable=no-name-in-module, import-outside-toplevel
  cmdline.set_author('Robert E. Smith (robert.smith@florey.edu.au)'
                     ' and David Raffelt (david.raffelt@florey.edu.au)')
  cmdline.set_synopsis('Calculate the mean response function from a set of text files')
  cmdline.add_description('All response function files provided must contain the same number of unique b-values (lines),'
                          ' as well as the same number of coefficients per line.')
  cmdline.add_description('As long as the number of unique b-values is identical across all input files,'
                          ' the response functions will be averaged.'
                          ' This is performed on the assumption that the actual acquired b-values are identical.'
                          ' This is however impossible for the responsemean command to determine based on the data provided;'
                          ' it is therefore up to the user to ensure that this requirement is satisfied.')
  cmdline.add_example_usage('Usage where all response functions are in the same directory:',
                            'responsemean input_response1.txt input_response2.txt input_response3.txt output_average_response.txt')
  cmdline.add_example_usage('Usage selecting response functions within a directory using a wildcard:',
                            'responsemean input_response*.txt output_average_response.txt')
  cmdline.add_example_usage('Usage where data for each participant reside in a participant-specific directory:',
                            'responsemean subject-*/response.txt output_average_response.txt')
  cmdline.add_argument('inputs',
                       type=app.Parser.FileIn(),
                       nargs='+',
                       help='The input response function files')
  cmdline.add_argument('output',
                       type=app.Parser.FileOut(),
                       help='The output mean response function file')
  cmdline.add_argument('-legacy',
                       action='store_true',
                       default=None,
                       help='Use the legacy behaviour of former command "average_response":'
                            ' average response function coefficients directly,'
                            ' without compensating for global magnitude differences between input files')



def execute(): #pylint: disable=unused-variable
  from mrtrix3 import MRtrixError #pylint: disable=no-name-in-module, import-outside-toplevel
  from mrtrix3 import app, matrix #pylint: disable=no-name-in-module, import-outside-toplevel

  data = [ ] # 3D matrix: Subject, b-value, ZSH coefficient
  for filepath in app.ARGS.inputs:
    subject = matrix.load_matrix(filepath)
    if any(len(line) != len(subject[0]) for line in subject[1:]):
      raise MRtrixError(f'File "{filepath}" does not contain the same number of entries per line '
                        f'(multi-shell response functions must have the same number of coefficients per b-value; '
                        f'pad the data with zeroes if necessary)')
    if data:
      if len(subject) != len(data[0]):
        raise MRtrixError(f'File "{filepath}" contains {len(subject)} {"b-values" if len(subject) > 1 else "bvalue"}) {"lines" if len(subject) > 1 else ""}; '
                          f'this differs from the first file read ({sys.argv[1]}), '
                          f'which contains {len(data[0])} {"lines" if len(data[0]) > 1 else ""}')
      if len(subject[0]) != len(data[0][0]):
        raise MRtrixError(f'File "{filepath}" contains {len(subject[0])} {"coefficients" if len(subject[0]) > 1 else ""} per b-value (line); '
                          f'this differs from the first file read ({sys.argv[1]}), '
                          f'which contains {len(data[0][0])} {"coefficients" if len(data[0][0]) > 1 else ""} per line')
    data.append(subject)

  app.console(f'Calculating mean RF across {len(data)} inputs, '
              f'with {len(data[0])} b-value{"s" if len(data[0])>1 else ""} '
              f'and lmax={2*(len(data[0][0])-1)}')

  # Old approach: Just take the average across all subjects
  # New approach: Calculate a multiplier to use for each subject, based on the geometric mean
  #   scaling factor required to bring the subject toward the group mean l=0 terms (across shells)

  mean_lzero_terms = [ sum(subject[row][0] for subject in data)/len(data) for row in range(len(data[0])) ]
  app.debug(f'Mean l=0 terms: {mean_lzero_terms}')

  weighted_sum_coeffs = [[0.0] * len(data[0][0]) for _ in range(len(data[0]))] #pylint: disable=unused-variable
  for subject in data:
    if app.ARGS.legacy:
      multiplier = 1.0
    else:
      subj_lzero_terms = [line[0] for line in subject]
      log_multiplier = 0.0
      for subj_lzero, mean_lzero in zip(subj_lzero_terms, mean_lzero_terms):
        log_multiplier += math.log(mean_lzero / subj_lzero)
      log_multiplier /= len(data[0])
      multiplier = math.exp(log_multiplier)
      app.debug(f'Subject l=0 terms: {subj_lzero_terms}')
      app.debug(f'Resulting multipler: {multiplier}')
    weighted_sum_coeffs = [ [ a + multiplier*b for a, b in zip(linea, lineb) ] for linea, lineb in zip(weighted_sum_coeffs, subject) ]

  mean_coeffs = [ [ f/len(data) for f in line ] for line in weighted_sum_coeffs ]
  matrix.save_matrix(app.ARGS.output, mean_coeffs, force=app.FORCE_OVERWRITE)
