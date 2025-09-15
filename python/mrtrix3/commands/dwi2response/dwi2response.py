# Copyright (c) 2008-2025 the MRtrix3 contributors.
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

  cmdline.set_author('Robert E. Smith (robert.smith@florey.edu.au)'
                     ' and Thijs Dhollander (thijs.dhollander@gmail.com)')
  cmdline.set_synopsis('Estimate response function(s) for spherical deconvolution')
  cmdline.add_description('dwi2response offers different algorithms for performing various types of response function estimation. '
                          'The name of the algorithm must appear as the first argument on the command-line after "dwi2response". '
                          'The subsequent arguments and options depend on the particular algorithm being invoked.')
  cmdline.add_description('Each algorithm available has its own help page,'
                          ' including necessary references;'
                          ' e.g. to see the help page of the "fa" algorithm,'
                           ' type "dwi2response fa".')
  cmdline.add_description('More information on response function estimation for spherical deconvolution'
                          ' can be found at the following link: \n'
                          f'https://mrtrix.readthedocs.io/en/{version.TAG}/constrained_spherical_deconvolution/response_function_estimation.html')
  cmdline.add_description('Note that if the -mask command-line option is not specified,'
                          ' the MRtrix3 command dwi2mask will automatically be called to'
                          ' derive an initial voxel exclusion mask.'
                          ' More information on mask derivation from DWI data can be found at: \n'
                          f'https://mrtrix.readthedocs.io/en/{version.TAG}/dwi_preprocessing/masking.html')

  # General options
  common_options = cmdline.add_argument_group('General dwi2response options')
  common_options.add_argument('-mask',
                              type=app.Parser.ImageIn(),
                              help='Provide an initial mask for response voxel selection')
  common_options.add_argument('-voxels',
                              type=app.Parser.ImageOut(),
                              help='Output an image showing the final voxel selection(s)')
  common_options.add_argument('-shells',
                              type=app.Parser.SequenceFloat(),
                              metavar='bvalues',
                              help='The b-value(s) to use in response function estimation '
                                   '(comma-separated list in case of multiple b-values; '
                                   'b=0 must be included explicitly if desired)')
  common_options.add_argument('-lmax',
                              type=app.Parser.SequenceInt(),
                              help='The maximum harmonic degree(s) for response function estimation '
                                   '(comma-separated list in case of multiple b-values)')
  app.add_dwgrad_import_options(cmdline)

  # Import the command-line settings for all algorithms found in the relevant directory
  cmdline.add_subparsers()






def execute(): #pylint: disable=unused-variable
  from mrtrix3 import CONFIG, MRtrixError #pylint: disable=no-name-in-module, import-outside-toplevel
  from mrtrix3 import app, image, run #pylint: disable=no-name-in-module, import-outside-toplevel

  # Load module for the user-requested algorithm
  alg = importlib.import_module(f'.{app.ARGS.algorithm}', 'mrtrix3.commands.dwi2response')

  # Sanitise some inputs, and get ready for data import
  if app.ARGS.lmax:
    if any(lmax%2 for lmax in app.ARGS.lmax):
      raise MRtrixError('Value(s) of lmax must be even')
    if alg.NEEDS_SINGLE_SHELL and not len(app.ARGS.lmax) == 1:
      raise MRtrixError('Can only specify a single lmax value for single-shell algorithms')
  shells_option = []
  if app.ARGS.shells:
    if alg.NEEDS_SINGLE_SHELL and len(app.ARGS.shells) != 1:
      raise MRtrixError('Can only specify a single b-value shell for single-shell algorithms')
    shells_option = ['-shells', ','.join(map(str,app.ARGS.shells))]
  singleshell_option = []
  if alg.NEEDS_SINGLE_SHELL:
    singleshell_option = ['-singleshell', '-no_bzero']

  grad_import_option = app.dwgrad_import_options()
  if not grad_import_option and 'dw_scheme' not in image.Header(app.ARGS.input).keyval():
    raise MRtrixError('Script requires diffusion gradient table: '
                      'either in image header, or using -grad / -fslgrad option')

  app.activate_scratch_dir()
  # Get standard input data into the scratch directory
  if alg.NEEDS_SINGLE_SHELL or shells_option:
    app.console(f'Importing DWI data ({app.ARGS.input}) and selecting b-values...')
    run.command(['mrconvert', app.ARGS.input, '-', '-strides', '0,0,0,1']
                + grad_import_option
                + ['|', 'dwiextract', '-', 'dwi.mif']
                + shells_option
                + singleshell_option,
                show=False,
                preserve_pipes=True)
  else: # Don't discard b=0 in multi-shell algorithms
    app.console(f'Importing DWI data ({app.ARGS.input})...')
    run.command(['mrconvert', app.ARGS.input, 'dwi.mif', '-strides', '0,0,0,1']
                + grad_import_option,
                show=False,
                preserve_pipes=True)
  if app.ARGS.mask:
    app.console(f'Importing mask ({app.ARGS.mask})...')
    run.command(['mrconvert', app.ARGS.mask, 'mask.mif', '-datatype', 'bit'],
                show=False,
                preserve_pipes=True)

  if alg.SUPPORTS_MASK:
    if app.ARGS.mask:
      # Check that the brain mask is appropriate
      mask_header = image.Header('mask.mif')
      if mask_header.size()[:3] != image.Header('dwi.mif').size()[:3]:
        raise MRtrixError('Dimensions of provided mask image do not match DWI')
      if not (len(mask_header.size()) == 3 or (len(mask_header.size()) == 4 and mask_header.size()[3] == 1)):
        raise MRtrixError('Provided mask image needs to be a 3D image')
    else:
      dwi2mask_algo = CONFIG['Dwi2maskAlgorithm']
      app.console(f'Computing brain mask (dwi2mask {dwi2mask_algo})...')
      run.command(f'dwi2mask {dwi2mask_algo} dwi.mif mask.mif', show=False)

    if not image.statistics('mask.mif', mask='mask.mif').count:
      raise MRtrixError(('Provided' if app.ARGS.mask else 'Generated') + ' mask image does not contain any voxels')

  # From here, the script splits depending on what estimation algorithm is being used
  alg.execute()
