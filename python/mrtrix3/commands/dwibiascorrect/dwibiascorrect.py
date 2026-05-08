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

import importlib



def usage(cmdline): #pylint: disable=unused-variable
  from mrtrix3 import app, version #pylint: disable=no-name-in-module, import-outside-toplevel
  cmdline.set_author('Robert E. Smith (robert.smith@florey.edu.au)')
  cmdline.set_synopsis('Perform B1 field inhomogeneity correction for a DWI volume series')
  cmdline.add_description('Note that if the -mask command-line option is not specified, '
                          'the MRtrix3 command dwi2mask will automatically be called to '
                          'derive a mask that will be passed to the relevant bias field estimation command. '
                          'More information on mask derivation from DWI data can be found at the following link: \n'
                          f'https://mrtrix.readthedocs.io/en/{version.TAG}/dwi_preprocessing/masking.html')
  common_options = cmdline.add_argument_group('Options common to all dwibiascorrect algorithms')
  common_options.add_argument('-mask',
                              type=app.Parser.ImageIn(),
                              help='Manually provide an input mask image for bias field estimation')
  common_options.add_argument('-bias',
                              type=app.Parser.ImageOut(),
                              help='Output an image containing the estimated bias field')
  app.add_dwgrad_import_options(cmdline)

  # Import the command-line settings for all algorithms found in the relevant directory
  cmdline.add_subparsers()



def execute(): #pylint: disable=unused-variable
  from mrtrix3 import CONFIG, MRtrixError #pylint: disable=no-name-in-module, import-outside-toplevel
  from mrtrix3 import app, image, run #pylint: disable=no-name-in-module, import-outside-toplevel

  # Load module for the user-requested algorithm
  alg = importlib.import_module(f'.{app.ARGS.algorithm}', 'mrtrix3.commands.dwibiascorrect')

  app.activate_scratch_dir()
  run.command(['mrconvert', app.ARGS.input, 'in.mif']
              + app.dwgrad_import_options(),
              preserve_pipes=True)
  if app.ARGS.mask:
    run.command(['mrconvert', app.ARGS.mask, 'mask.mif', '-datatype', 'bit'],
                preserve_pipes=True)

  # Make sure it's actually a DWI that's been passed
  dwi_header = image.Header('in.mif')
  if len(dwi_header.size()) != 4:
    raise MRtrixError('Input image must be a 4D image')
  if 'dw_scheme' not in dwi_header.keyval():
    raise MRtrixError('No valid DW gradient scheme provided or present in image header')
  dwscheme_rows = len(dwi_header.keyval()['dw_scheme'])
  if dwscheme_rows != dwi_header.size()[3]:
    raise MRtrixError(f'DW gradient scheme contains different number of entries'
                      f' ({dwscheme_rows})'
                      f' to number of volumes in DWIs'
                      f' ({dwi_header.size()[3]})')

  # Generate a brain mask if required, or check the mask if provided by the user
  if app.ARGS.mask:
    if not image.match('in.mif', 'mask.mif', up_to_dim=3):
      raise MRtrixError('Provided mask image does not match input DWI')
  else:
    run.command(['dwi2mask', CONFIG['Dwi2maskAlgorithm'], 'in.mif', 'mask.mif'])

  # From here, the script splits depending on what estimation algorithm is being used
  alg.execute()
