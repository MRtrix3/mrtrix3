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

def getreference(isboolean, is1D):
    if isboolean:
        return REFERENCE_1D_BOOL if is1D else REFERENCE_2D_BOOL
    else:
        return REFERENCE_1D if is1D else REFERENCE_2D

for entry in os.listdir(dirname):
    fullpath = os.path.join(dirname, entry)

    print (entry)
    data = np.load(fullpath)

    isboolean = 'BOOL' in entry
    is1D = entry.startswith('1D')

    if not np.array_equal(data, getreference(isboolean, is1D)):
        errors.append(entry)

if errors:
    raise Exception(str(errors.size()) + ' errors detected: ' + str(errors))
