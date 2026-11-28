#!/usr/bin/env python

# Copyright (c) 2008-2026 the MRtrix3 contributors.
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


import math, os, sys



def usage(cmdline): #pylint: disable=unused-variable
  from mrtrix3 import app
  cmdline.set_author('Robert E. Smith (robert.smith@florey.edu.au)'
                      ' and Kinkini Monaragala (kinkini.monaragala@utoronto.ca)')
  cmdline.set_synopsis('Calculate the SNR from a given DWI file')
  cmdline.add_description('Example usage: dwi.mif snr_output.mif -method b0_stdev')
  # add in description here
  cmdline.add_argument('input', help='The input DWI file')
  cmdline.add_argument('output', help='The output snr file')
  # algorithm selection
  methods = cmdline.add_argument_group('SNR estimation methods')
  methods.add_argument('-method', choices=['b0_stdev', 'noise_level', 'sh_residuals'],
                       default='sh_residuals',
                       help='''SNR estimation methods:
                               "b0_stdev": Ratio of mean b=0 to stdev of b=0;
                               "noise_level": Uses the noise estimate from dwidenoise;
                               "sh_residuals": Uses residuals from dpherical harmonic (SH) fits''')
  #common_options.add_argument('-shells', help='The b-value(s) to use in response function estimation (comma-separated list in case of multiple b-values, b=0 must be included explicitly)')


def execute():
    from mrtrix3 import MRtrixError
    from mrtrix3 import app, path, matrix, run

    #app.check_output_path(app.ARGS.output)
    app.activate_scratch_dir()

    # option 1 Ratio of mean b=0 to stdev of b=0 intensities
    if app.ARGS.method == 'b0_stdev':
        #print(np.shape(dwi))

        # what is the best way to incoporate more than one bzero vlaue
        run.command(f'dwiextract {app.ARGS.input} b0s.mif -bzero')

        # calculate mean and standard deviation along axis 3
        run.command('mrmath b0s.mif mean mean.mif -axis 3')
        run.command('mrmath b0s.mif std std.mif -axis 3')

        # smoothing to produce a better noise estimate
        run.command('mrfilter mean.mif smooth mean_s.mif -stdev 1')
        run.command('mrfilter std.mif smooth std_s.mif -stdev 1')

        # ratio of mean / std
        run.command(['mrcalc', 'mean_s.mif', 'std_s.mif', '-divide', app.ARGS.output, '-force'], force=app.FORCE_OVERWRITE)

    ## add more options here

