#!/usr/bin/python3

import os
import sys

try:
  import numpy as np
except ImportError:
  sys.stderr.write('WARNING: Unable to import numpy module; validity of .npy files will not be checked\n')
  sys.exit(0)

REFERENCE_1D = np.arange(0, 3)
REFERENCE_1D_BOOL = REFERENCE_1D.astype('?')
REFERENCE_2D = np.array([[0,1], [10,11], [20,21]])
REFERENCE_2D_BOOL = REFERENCE_2D.astype('?')

dirname = sys.argv[1]

errors = []

def getreference(boolean, onedim):
  if boolean:
    return REFERENCE_1D_BOOL if onedim else REFERENCE_2D_BOOL
  return REFERENCE_1D if onedim else REFERENCE_2D

for entry in os.listdir(dirname):
  fullpath = os.path.join(dirname, entry)

  print (entry)
  data = np.load(fullpath)

  isboolean = 'BOOL' in entry
  is1d = entry.startswith('1D')

  if not np.array_equal(data, getreference(isboolean, is1d)):
    errors.append(entry)

if errors:
  raise RuntimeError(f'{len(errors)} errors detected: {errors}')
