#!/usr/bin/env python

import argparse
import math
import numpy as np
from scipy.linalg import logm, expm
from scipy.signal import medfilt


def getsliceorder(n, p=2, s=1):
    return np.array([j for k in range(0,p) for j in range((k*s)%p,n,p)], dtype=int)


if __name__ == '__main__':
    # arguments
    parser = argparse.ArgumentParser(description='Filtering a series of rigid motion parameters.')
    parser.add_argument('input', type=str, help='input motion file')
    parser.add_argument('weights', type=str, help='input weight matrix')
    parser.add_argument('output', type=str, help='output motion file')
    #parser.add_argument('-mb', type=int, default=1, help='multiband factor')
    parser.add_argument('-packs', type=int, default=2, help='no. slice packs')
    parser.add_argument('-shift', type=int, default=1, help='slice shift')
    parser.add_argument('-medfilt', type=int, default=1, help='median filtering kernel size (default = 1; disabled)')
    parser.add_argument('-driftfilt', action='store_true', help='drift filter slice packs')
    # mandatory MRtrix options (unused)
    parser.add_argument('-nthreads', type=int, default=1, help='no. threads (unused)')
    parser.add_argument('-info', help='(unused)')
    parser.add_argument('-debug', help='(unused)')
    parser.add_argument('-quiet', help='(unused)')
    parser.add_argument('-force', help='(unused)')
    args = parser.parse_args()
    # read inputs
    M = np.loadtxt(args.input)
    W = np.clip(np.loadtxt(args.weights), 1e-6, None)
    # set up slice order
    nv = W.shape[1]
    ne = M.shape[0]//nv
    sliceorder = getsliceorder(ne, args.packs, args.shift)
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
        M2 = medfilt(M2, (args.medfilt, 1))
    if args.driftfilt:
        M2 = M2.reshape((nv,ne,6)) - np.median(M2.reshape((nv,ne,6)), 0)
    # reorder output
    M3 = np.reshape(M2.reshape((nv,ne,6))[:,isliceorder,:], (-1,6))
    np.savetxt(args.output, M3, fmt='%.6f')


