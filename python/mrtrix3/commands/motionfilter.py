#   Copyright (c) 2017-2019 Daan Christiaens
#
#   MRtrix and this add-on module are distributed in the hope
#   that it will be useful, but WITHOUT ANY WARRANTY; without
#   even the implied warranty of MERCHANTABILITY or FITNESS
#   FOR A PARTICULAR PURPOSE.
#
#   Author:  Daan Christiaens
#            King's College London
#            daan.christiaens@kcl.ac.uk
#

import numpy as np
from scipy.linalg import logm, expm
from scipy.signal import medfilt


def getsliceorder(n, p=2, s=1):
    return np.array([j for k in range(0,p) for j in range((k*s)%p,n,p)], dtype=int)


def usage(cmdline): #pylint: disable=unused-variable
    from mrtrix3 import app #pylint: disable=no-name-in-module, import-outside-toplevel
    cmdline.set_author('Daan Christiaens (daan.christiaens@kcl.ac.uk)')
    cmdline.set_synopsis('Filtering a series of rigid motion parameters')
    cmdline.add_description('This command applies a filter on a timeseries of rigid motion parameters.'
                            ' This is used in dwimotioncorrect to correct severe registration errors.')
    cmdline.add_argument('input',
                         type=app.Parser.FileIn(),
                         help='The input motion file')
    cmdline.add_argument('weights',
                         type=app.Parser.FileIn(),
                         help='The input weight matrix')
    cmdline.add_argument('output',
                         type=app.Parser.FileOut(),
                         help='The output motion file')
    cmdline.add_argument('-packs', type=int, default=2, help='no. slice packs')
    cmdline.add_argument('-shift', type=int, default=1, help='slice shift')
    cmdline.add_argument('-medfilt', type=int, default=1, help='median filtering kernel size (default = 1; disabled)')
    cmdline.add_argument('-driftfilt', action='store_true', help='drift filter slice packs')


def execute(): #pylint: disable=unused-variable
    from mrtrix3 import MRtrixError #pylint: disable=no-name-in-module, import-outside-toplevel
    from mrtrix3 import app, image #pylint: disable=no-name-in-module, import-outside-toplevel
    # read inputs
    M = np.loadtxt(app.ARGS.input)
    W = np.clip(np.loadtxt(app.ARGS.weights), 1e-6, None)
    # set up slice order
    nv = W.shape[1]
    ne = M.shape[0]//nv
    sliceorder = getsliceorder(ne, app.ARGS.packs, app.ARGS.shift)
    isliceorder = np.argsort(sliceorder)
    # reorder
    M1 = np.reshape(M.reshape((nv,ne,6))[:,sliceorder,:], (-1,6))
    W1 = np.reshape(np.mean(W.reshape((-1, ne, nv)), axis=0)[sliceorder,:], (-1,1));
    # filter: t'_i = w * t_i + (1-w) * (t'_(i-1) + t'_(i+1)) / 2
    A = np.eye(nv*ne) - 0.5 * (1.-W1) * (np.eye(nv*ne, k=1) + np.eye(nv*ne, k=-1))
    A[0,1] = W1[0] - 1
    A[-1,-2] = W1[-1] - 1
    M2 = np.linalg.solve(A, W1 * M1)
    # median filtering
    if ne > 1:
        M2 = medfilt(M2, (app.ARGS.medfilt, 1))
    if app.ARGS.driftfilt:
        M2 = M2.reshape((nv,ne,6)) - np.median(M2.reshape((nv,ne,6)), 0)
    # reorder output
    M3 = np.reshape(M2.reshape((nv,ne,6))[:,isliceorder,:], (-1,6))
    np.savetxt(app.ARGS.output, M3, fmt='%.6f')

