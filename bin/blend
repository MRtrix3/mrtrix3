#!/usr/bin/python3

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

import os
import sys

if len(sys.argv) <= 1:
  sys.stderr.write('A script to blend two sets of movie frames together with a desired overlap.\n')
  sys.stderr.write('The input arguments are two folders containing the movie frames '
                   '(eg. output from the MRview screenshot tool), and the desired number '
                   'of overlapping frames.\n')
  sys.stderr.write('eg: blend folder1 folder2 20 output_folder\n')
  sys.exit(1)

INPUT_FOLDER_1 = sys.argv[1]
INPUT_FOLDER_2 = sys.argv[2]
FILE_LIST_1 = sorted(os.listdir(INPUT_FOLDER_1))
FILE_LIST_2 = sorted(os.listdir(INPUT_FOLDER_2))
NUM_OVERLAP = int(sys.argv[3])
OUTPUT_FOLDER = sys.argv[4]

if not os.path.exists(OUTPUT_FOLDER):
  os.mkdir(OUTPUT_FOLDER)

NUM_OUTPUT_FRAMES = len(FILE_LIST_1) + len(FILE_LIST_2) - NUM_OVERLAP
for i in range(NUM_OUTPUT_FRAMES):
  file_name = 'frame' + '%0*d' % (5, i) + '.png'
  if i <= len(FILE_LIST_1) - NUM_OVERLAP:
    os.system('cp -L ' + INPUT_FOLDER_1 + '/' + FILE_LIST_1[i] + ' ' + OUTPUT_FOLDER + '/' + file_name)
  if len(FILE_LIST_1) - NUM_OVERLAP < i < len(FILE_LIST_1):
    i2 = i - (len(FILE_LIST_1) - NUM_OVERLAP) - 1
    blend_amount = 100 * float(i2 + 1) / float(NUM_OVERLAP)
    os.system('convert ' + INPUT_FOLDER_1 + '/' + FILE_LIST_1[i] + ' ' + INPUT_FOLDER_2 \
        + '/' + FILE_LIST_2[i2] + ' -alpha on -compose blend  -define compose:args=' \
        + str(blend_amount) + ' -gravity South  -composite ' + OUTPUT_FOLDER + '/' + file_name)
  if i >= (len(FILE_LIST_1)):
    i2 = i - (len(FILE_LIST_1) - NUM_OVERLAP) - 1
    os.system('cp -L ' + INPUT_FOLDER_2 + '/' + FILE_LIST_2[i2] + ' ' + OUTPUT_FOLDER + '/' + file_name)
