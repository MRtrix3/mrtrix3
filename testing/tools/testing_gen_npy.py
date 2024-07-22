#!/usr/bin/python3

import os
import sys

dirname = sys.argv[1]
if not os.path.exists(dirname):
    os.makedirs(dirname)

try:
    import numpy as np
except ImportError:
    sys.stderr.write('WARNING: Error importing numpy module; cannot conduct npy unit tests\n')
    sys.exit(0)

dtypes = ['?', 'b', 'B']
for bytes in [2, 4, 8]:
    for kind in ['i', 'u', 'f']:
        for endianness in ['<', '>']:
            dtypes.append(endianness + kind + str(bytes))

def genpath(prefix, ord, dt):
    return os.path.join(dirname, prefix + '_' + ord + '_' + dt.replace('<', 'LE').replace('>', 'BE').replace('?', 'BOOL') + '.npy')

for dtype in dtypes:
    for order in ['C', 'F']:

        # 1D data
        data = np.empty((3), dtype, order=order)
        for index in range(0, 3):
            data[index] = index
        np.save(genpath('1D3', order, dtype), data)

        # Column vector
        data = np.empty((3, 1), dtype, order=order)
        for index in range(0, 3):
            data[index, 0] = index
        np.save(genpath('1D3x1', order, dtype), data)

        # Row vector
        data = np.empty((1, 3), dtype, order=order)
        for index in range(0, 3):
            data[0, index] = index
        np.save(genpath('1D1x3', order, dtype), data)

        # 2D matrix
        data = np.empty((3, 2), dtype, order=order)
        for row in range(0, 3):
            for col in range(0, 2):
                data[row, col] = 10*row + col
        np.save(genpath('2D3x2', order, dtype), data)
