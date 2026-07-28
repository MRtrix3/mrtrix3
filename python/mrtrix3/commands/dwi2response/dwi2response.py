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

  cmdline.set_author('Robert E. Smith (robert.smith@florey.edu.au)'
                     ' and Thijs Dhollander (thijs.dhollander@gmail.com)')
  cmdline.set_synopsis('Estimate response function(s) for spherical deconvolution')
  cmdline.add_description('dwi2response offers different algorithms for performing various types of response function estimation. '
                          'The name of the sub-command must appear as the first argument on the command-line after "dwi2response". '
                          'The subsequent arguments and options depend on the particular sub-command being invoked.')
  cmdline.add_description('Each sub-command has its own help page,'
                          ' including necessary references;'
                          ' e.g. to see the help page of the "fa" sub-command,'
                           ' type "dwi2response fa".')
  cmdline.add_description('More information on response function estimation for spherical deconvolution'
                          ' can be found at the following link: \n'
                          f'https://mrtrix.readthedocs.io/en/{version.TAG}/constrained_spherical_deconvolution/response_function_estimation.html')
  cmdline.add_description('Note that if the -mask command-line option is not specified,'
                          ' the MRtrix3 command dwi2mask will automatically be called to'
                          ' derive an initial voxel exclusion mask.'
                          ' More information on mask derivation from DWI data can be found at: \n'
                          f'https://mrtrix.readthedocs.io/en/{version.TAG}/dwi_preprocessing/masking.html')
  cmdline.add_description('In the absence of a user-specified mask (option -mask),'
                          ' the whole DWI series will be used for derivation of the brain mask,'
                          ' even where only a subset of the DWI volumes is used for response function estimation'
                          ' (whether because the -shells option has been specified,'
                          ' or because the nominated algorithm operates on only a single b-value shell).'
                          ' If it is instead desired that the same subset of shells used for response function estimation'
                          ' also be used for brain mask derivation,'
                          ' then the user has two alternatives:'
                          ' either generate that subset of shells themselves---'
                          'e.g. using the dwiextract command---'
                          'and provide the result as the input to dwi2response;'
                          ' or generate a brain mask from that subset of shells'
                          ' and provide that mask via the -mask option.')

  # General options
  common_options = cmdline.add_option_group('General dwi2response options')
  common_options.add_option('mask',
                            'Provide an initial mask for response voxel selection',
                            type=app.Parser.ImageIn())
  common_options.add_option('voxels',
                            'Output an image showing the final voxel selection(s)',
                            type=app.Parser.ImageOut())
  common_options.add_option('shells',
                            'The b-value(s) to use in response function estimation '
                            '(comma-separated list in case of multiple b-values; '
                            'b=0 must be included explicitly if desired)',
                            type=app.Parser.SequenceFloat(),
                            metavar='bvalues')
  common_options.add_option('lmax',
                            'The maximum harmonic degree(s) for response function estimation '
                            '(comma-separated list in case of multiple b-values)',
                            type=app.Parser.SequenceLmax())
  app.add_dwgrad_import_options(cmdline)

  # Import the command-line settings for all algorithms found in the relevant directory
  cmdline.add_subcommands()






def execute(): #pylint: disable=unused-variable
  from mrtrix3 import CONFIG, MRtrixError #pylint: disable=no-name-in-module, import-outside-toplevel
  from mrtrix3 import app, image, run #pylint: disable=no-name-in-module, import-outside-toplevel

  # Load module for the user-requested algorithm
  alg = importlib.import_module(f'.{app.ARGS.subcommand}', 'mrtrix3.commands.dwi2response')

  # Sanitise some inputs, and get ready for data import
  if app.ARGS.lmax:
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

  # Determine whether only a subset of the DWI volumes is to be used for response function estimation;
  #   this is the case if the user has explicitly requested a subset of shells,
  #   or if the nominated algorithm operates on only a single b-value shell.
  need_to_extract = bool(alg.NEEDS_SINGLE_SHELL or shells_option)
  # Both operands are command-line token lists, so this is a list concatenation, not a
  #   string formation: the result is spliced into the run.command() argument lists below.
  extract_option = shells_option + singleshell_option

  # Import the user-provided mask, if one was given
  if app.ARGS.mask:
    app.console(f'Importing mask ({app.ARGS.mask})...')
    run.command(['mrconvert', app.ARGS.mask, 'mask.mif', '-datatype', 'bit'],
                show=False,
                preserve_pipes=True)

  # Get standard input data into the scratch directory
  if need_to_extract and alg.SUPPORTS_MASK and not app.ARGS.mask:
    # The user has requested that only a subset of the data be used for response function estimation
    #   (or the nominated algorithm operates on only a single shell),
    #   but no mask has been provided and therefore one must be derived.
    # Import the whole DWI series so that brain mask derivation can make use of all available data,
    #   derive the mask, and only then extract the requested subset of shells
    #   for the purpose of response function estimation.
    app.console(f'Importing DWI data ({app.ARGS.input})...')
    run.command(['mrconvert', app.ARGS.input, 'dwi_full.mif', '-strides', '0,0,0,1']
                + grad_import_option,
                show=False,
                preserve_pipes=True)
    dwi2mask_algo = CONFIG['Dwi2maskAlgorithm']
    app.console(f'Computing brain mask (dwi2mask {dwi2mask_algo})...')
    run.command(f'dwi2mask {dwi2mask_algo} dwi_full.mif mask.mif', show=False)
    app.console('Extracting requested b-value shells for response function estimation...')
    run.command(['dwiextract', 'dwi_full.mif', 'dwi.mif'] + extract_option,
                show=False)
    app.cleanup('dwi_full.mif')
  elif need_to_extract:
    # Either a mask has been provided by the user, or the nominated algorithm makes no use of a mask;
    #   the requested subset of shells can therefore be extracted immediately upon import.
    app.console(f'Importing DWI data ({app.ARGS.input}) and selecting b-values...')
    run.command(['mrconvert', app.ARGS.input, '-', '-strides', '0,0,0,1']
                + grad_import_option
                + ['|', 'dwiextract', '-', 'dwi.mif']
                + extract_option,
                show=False,
                preserve_pipes=True)
  else: # Don't discard b=0 in multi-shell algorithms
    app.console(f'Importing DWI data ({app.ARGS.input})...')
    run.command(['mrconvert', app.ARGS.input, 'dwi.mif', '-strides', '0,0,0,1']
                + grad_import_option,
                show=False,
                preserve_pipes=True)
    if alg.SUPPORTS_MASK and not app.ARGS.mask:
      dwi2mask_algo = CONFIG['Dwi2maskAlgorithm']
      app.console(f'Computing brain mask (dwi2mask {dwi2mask_algo})...')
      run.command(f'dwi2mask {dwi2mask_algo} dwi.mif mask.mif', show=False)

  if alg.SUPPORTS_MASK:
    if app.ARGS.mask:
      # Check that the brain mask is appropriate
      mask_header = image.Header('mask.mif')
      if mask_header.size()[:3] != image.Header('dwi.mif').size()[:3]:
        raise MRtrixError('Dimensions of provided mask image do not match DWI')
      if not (len(mask_header.size()) == 3 or (len(mask_header.size()) == 4 and mask_header.size()[3] == 1)):
        raise MRtrixError('Provided mask image needs to be a 3D image')

    if not image.statistics('mask.mif', mask='mask.mif').count:
      raise MRtrixError(f'{"Provided" if app.ARGS.mask else "Generated"} mask image does not contain any voxels')

  # From here, the script splits depending on what estimation algorithm is being used
  alg.execute()
