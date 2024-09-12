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
import matplotlib.pyplot as plt


def getsliceorder(n, p=2, s=1):
    return np.array([j for k in range(0,p) for j in range((k*s)%p,n,p)], dtype=int)


def tr2lie(T):
    r = np.zeros((6,))
    try:
        L = logm(T)
        R = 0.5 * (L - L.T)
        r[:3] = L[:3,3]
        r[3:] = R[2,1], R[0,2], R[1,0]
    except:
        pass
    return r


def lie2tr(r):
    L = np.zeros((4,4))
    L[:3,3] = r[:3]
    L[2,1] = r[3] ; L[1,2] = -r[3]
    L[0,2] = r[4] ; L[2,0] = -r[4]
    L[1,0] = r[5] ; L[0,1] = -r[5]
    T = expm(L)
    return T


def tr2euler(T):
    sy = math.hypot(T[0,0], T[1,0])
    if sy > 1e-6:
        roll = math.atan2(T[2,1], T[2,2])
        pitch = math.atan2(-T[2,0], sy)
        yaw = math.atan2(T[1,0], T[0,0])
    else:
        roll = math.atan2(-T[1,2], T[1,1])
        pitch = math.atan2(-T[2,0], sy)
        yaw = 0.0
    return np.array([T[0,3], T[1,3], T[2,3], yaw, pitch, roll])


def usage(cmdline): #pylint: disable=unused-variable
    from mrtrix3 import app #pylint: disable=no-name-in-module, import-outside-toplevel
    cmdline.set_author('Daan Christiaens (daan.christiaens@kcl.ac.uk)')
    cmdline.set_synopsis('Calculate motion and outlier statistics')
    cmdline.add_description('This command calculates statistics of the subject translation and rotation'
                            ' and of the detected slice outliers.')
    cmdline.add_argument('input',
                         type=app.Parser.FileIn(),
                         help='The input motion file')
    cmdline.add_argument('weights',
                         type=app.Parser.FileIn(),
                         help='The input weight matrix')
    cmdline.add_argument('-packs', type=int, default=2, help='no. slice packs')
    cmdline.add_argument('-shift', type=int, default=1, help='slice shift')
    cmdline.add_argument('-plot', action='store_true', help='plot motion trajectory')
    cmdline.add_argument('-grad', type=app.Parser.FileIn(), help='dMRI gradient table')
    cmdline.add_argument('-dispersion', type=app.Parser.FileIn(), help='output gradient dispersion to file')


def execute(): #pylint: disable=unused-variable
    from mrtrix3 import MRtrixError #pylint: disable=no-name-in-module, import-outside-toplevel
    from mrtrix3 import app, image #pylint: disable=no-name-in-module, import-outside-toplevel
    # read inputs
    M0 = np.loadtxt(app.ARGS.input)
    W = np.loadtxt(app.ARGS.weights)
    # set up slice order
    nv = W.shape[1]
    ne = M0.shape[0]//nv
    sliceorder = getsliceorder(ne, app.ARGS.packs, app.ARGS.shift)
    isliceorder = np.argsort(sliceorder)
    # reorder
    M = np.reshape(M0.reshape((nv,ne,6))[:,sliceorder,:], (-1,6))
    # motion stats
    dM = M[1:,:] - M[:-1,:]
    mtra = np.mean(np.sum(dM[:,:3]**2, axis=1))
    mrot = np.mean(np.sum(dM[:,3:]**2, axis=1)) * 1000.
    # outlier stats
    orratio = 1.0 - np.sum(W) / (W.shape[0] * W.shape[1])
    # print stats
    print('{:f} {:f} {:f}'.format(mtra, mrot, orratio))
    # plot trajectory
    if app.ARGS.plot:
        T = np.array([tr2euler(lie2tr(m)) for m in M])
        ax1 = plt.subplot(2,1,1); plt.plot(T[:,:3]); plt.ylabel('translation'); plt.legend(['x', 'y', 'z']);
        ax2 = plt.subplot(2,1,2, sharex=ax1); plt.plot(T[:,3:]); plt.ylabel('rotation'); plt.legend(['yaw', 'pitch', 'roll']);
        plt.xlim(0, nv*ne); plt.xlabel('time'); plt.tight_layout();
        plt.show();
    # intra-volume gradient scatter
    if app.ARGS.dispersion:
        if app.ARGS.grad:
            grad = np.loadtxt(app.ARGS.grad)
            bvec = grad[:,:3] / np.linalg.norm(grad[:,:3], axis=1)[:,np.newaxis]
            r = np.array([[np.dot(lie2tr(m)[:3,:3], v) for m in mvol] for mvol, v in zip(M.reshape((nv,ne,6)), bvec)])
            rm = np.sum(r, axis=1); rm /= np.linalg.norm(rm, axis=1)[:,np.newaxis]
            rd = np.einsum('vzi,vi->vz', r, rm)
            dispersion = np.degrees(2*np.arccos(np.sqrt(np.mean(rd**2, axis=1))))
            np.savetxt(app.ARGS.dispersion, dispersion[np.newaxis,:], fmt='%.4f')
        else:
            raise MRtrixError('No diffusion gradient table provided')

