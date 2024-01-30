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

from mrtrix3 import MRtrixError
from mrtrix3 import app, image, path, run


LMAXES_MULTI = [4, 0, 0]
LMAXES_SINGLE = [4, 0]

def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('mtnorm', parents=[base_parser])
  parser.set_author('Robert E. Smith (robert.smith@florey.edu.au) and Arshiya Sangchooli (asangchooli@student.unimelb.edu.au)')
  parser.set_synopsis('Perform DWI bias field correction using the "mtnormalise" command')
  parser.add_description('This algorithm bases its operation almost entirely on the utilisation of multi-tissue '
                         'decomposition information to estimate an underlying B1 receive field, as is implemented '
                         'in the MRtrix3 command "mtnormalise". Its typical usage is however slightly different, '
                         'in that the primary output of the command is not the bias-field-corrected FODs, but a '
                         'bias-field-corrected version of the DWI series.')
  parser.add_description('The operation of this script is a subset of that performed by the script "dwibiasnormmask". '
                         'Many users may find that comprehensive solution preferable; this dwibiascorrect algorithm is '
                         'nevertheless provided to demonstrate specifically the bias field correction portion of that command.')
  parser.add_description('The ODFs estimated within this optimisation procedure are by default of lower maximal spherical harmonic '
                         'degree than what would be advised for analysis. This is done for computational efficiency. This '
                         'behaviour can be modified through the -lmax command-line option.')
  parser.add_citation('Jeurissen, B; Tournier, J-D; Dhollander, T; Connelly, A & Sijbers, J. '
                      'Multi-tissue constrained spherical deconvolution for improved analysis of multi-shell diffusion MRI data. '
                      'NeuroImage, 2014, 103, 411-426')
  parser.add_citation('Raffelt, D.; Dhollander, T.; Tournier, J.-D.; Tabbara, R.; Smith, R. E.; Pierre, E. & Connelly, A. '
                      'Bias Field Correction and Intensity Normalisation for Quantitative Analysis of Apparent Fibre Density. '
                      'In Proc. ISMRM, 2017, 26, 3541')
  parser.add_citation('Dhollander, T.; Tabbara, R.; Rosnarho-Tornstrand, J.; Tournier, J.-D.; Raffelt, D. & Connelly, A. '
                      'Multi-tissue log-domain intensity and inhomogeneity normalisation for quantitative apparent fibre density. '
                      'In Proc. ISMRM, 2021, 29, 2472')
  parser.add_argument('input', type=app.Parser.ImageIn, help='The input image series to be corrected')
  parser.add_argument('output', type=app.Parser.ImageOut, help='The output corrected image series')
  options = parser.add_argument_group('Options specific to the "mtnorm" algorithm')
  options.add_argument('-lmax',
                       type=app.Parser.SequenceInt,
                       metavar='values',
                       help='The maximum spherical harmonic degree for the estimated FODs (see Description); '
                            'defaults are "' + ','.join(str(item) for item in LMAXES_MULTI) + '" for multi-shell and "' + ','.join(str(item) for item in LMAXES_SINGLE) + '" for single-shell data)')



def check_output_paths(): #pylint: disable=unused-variable
  pass



def get_inputs(): #pylint: disable=unused-variable
  pass



def execute(): #pylint: disable=unused-variable

  # Verify user inputs
  lmax = None
  if app.ARGS.lmax:
    try:
      lmax = [int(i) for i in app.ARGS.lmax.split(',')]
    except ValueError as exc:
      raise MRtrixError('Values provided to -lmax option must be a comma-separated list of integers') from exc
    if any(value < 0 or value % 2 for value in lmax):
      raise MRtrixError('lmax values must be non-negative even integers')
    if len(lmax) not in [2, 3]:
      raise MRtrixError('Length of lmax vector expected to be either 2 or 3')

  # Determine whether we are working with single-shell or multi-shell data
  bvalues = [
      int(round(float(value)))
      for value in image.mrinfo('in.mif', 'shell_bvalues') \
                               .strip().split()]
  multishell = (len(bvalues) > 2)
  if lmax is None:
    lmax = LMAXES_MULTI if multishell else LMAXES_SINGLE
  elif len(lmax) == 3 and not multishell:
    raise MRtrixError('User specified 3 lmax values for three-tissue decomposition, but input DWI is not multi-shell')

  # RF estimation and multi-tissue CSD
  class Tissue(object): #pylint: disable=useless-object-inheritance
    def __init__(self, name):
      self.name = name
      self.tissue_rf = 'response_' + name + '.txt'
      self.fod = 'FOD_' + name + '.mif'
      self.fod_norm = 'FODnorm_' + name + '.mif'

  tissues = [Tissue('WM'), Tissue('GM'), Tissue('CSF')]

  run.command('dwi2response dhollander in.mif'
              + (' -mask mask.mif' if app.ARGS.mask else '')
              + ' '
              + ' '.join(tissue.tissue_rf for tissue in tissues))

  # Immediately remove GM if we can't deal with it
  if not multishell:
    app.cleanup(tissues[1].tissue_rf)
    tissues = tissues[::2]

  run.command('dwi2fod msmt_csd in.mif'
              + ' -lmax ' + ','.join(str(item) for item in lmax)
              + ' '
              + ' '.join(tissue.tissue_rf + ' ' + tissue.fod
                         for tissue in tissues))

  run.command('maskfilter mask.mif erode - | '
              + 'mtnormalise -mask - -balanced'
              + ' -check_norm field.mif '
              + ' '.join(tissue.fod + ' ' + tissue.fod_norm
                          for tissue in tissues))
  app.cleanup([tissue.fod for tissue in tissues])
  app.cleanup([tissue.fod_norm for tissue in tissues])
  app.cleanup([tissue.tissue_rf for tissue in tissues])

  run.command('mrcalc in.mif field.mif -div - | '
              'mrconvert - '+ path.from_user(app.ARGS.output),
              mrconvert_keyval=path.from_user(app.ARGS.input, False),
              force=app.FORCE_OVERWRITE)

  if app.ARGS.bias:
    run.command('mrconvert field.mif ' + path.from_user(app.ARGS.bias),
                mrconvert_keyval=path.from_user(app.ARGS.input, False),
                force=app.FORCE_OVERWRITE)
