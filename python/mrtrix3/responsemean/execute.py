# Copyright (c) 2008-2023 the MRtrix3 contributors.
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
from mrtrix3 import MRtrixError #pylint: disable=no-name-in-module
from mrtrix3 import app, matrix #pylint: disable=no-name-in-module

def execute(): #pylint: disable=unused-variable

  app.check_output_path(app.ARGS.output)

  data = [ ] # 3D matrix: Subject, b-value, ZSH coefficient
  for filepath in app.ARGS.inputs:
    subject = matrix.load_matrix(filepath)
    if any(len(line) != len(subject[0]) for line in subject[1:]):
      raise MRtrixError('File \'' + filepath + '\' does not contain the same number of entries per line (multi-shell response functions must have the same number of coefficients per b-value; pad the data with zeroes if necessary)')
    if data:
      if len(subject) != len(data[0]):
        raise MRtrixError('File \'' + filepath + '\' contains ' + str(len(subject)) + ' b-value' + ('s' if len(subject) > 1 else '') + ' (line' + ('s' if len(subject) > 1 else '') + '); this differs from the first file read (' + sys.argv[1] + '), which contains ' + str(len(data[0])) + ' line' + ('s' if len(data[0]) > 1 else ''))
      if len(subject[0]) != len(data[0][0]):
        raise MRtrixError('File \'' + filepath + '\' contains ' + str(len(subject[0])) + ' coefficient' + ('s' if len(subject[0]) > 1 else '') + ' per b-value (line); this differs from the first file read (' + sys.argv[1] + '), which contains ' + str(len(data[0][0])) + ' coefficient' + ('s' if len(data[0][0]) > 1 else '') + ' per line')
    data.append(subject)

  app.console('Calculating mean RF across ' + str(len(data)) + ' inputs, with ' + str(len(data[0])) + ' b-value' + ('s' if len(data[0])>1 else '') + ' and lmax=' + str(2*(len(data[0][0])-1)))

  # Old approach: Just take the average across all subjects
  # New approach: Calculate a multiplier to use for each subject, based on the geometric mean
  #   scaling factor required to bring the subject toward the group mean l=0 terms (across shells)

  mean_lzero_terms = [ sum(subject[row][0] for subject in data)/len(data) for row in range(len(data[0])) ]
  app.debug('Mean l=0 terms: ' + str(mean_lzero_terms))

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
      app.debug('Subject l=0 terms: ' + str(subj_lzero_terms))
      app.debug('Resulting multipler: ' + str(multiplier))
    weighted_sum_coeffs = [ [ a + multiplier*b for a, b in zip(linea, lineb) ] for linea, lineb in zip(weighted_sum_coeffs, subject) ]

  mean_coeffs = [ [ f/len(data) for f in line ] for line in weighted_sum_coeffs ]
  matrix.save_matrix(app.ARGS.output, mean_coeffs, force=app.FORCE_OVERWRITE)
