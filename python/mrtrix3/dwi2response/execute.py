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

from mrtrix3 import CONFIG, MRtrixError #pylint: disable=no-name-in-module
from mrtrix3 import algorithm, app, image, path, run #pylint: disable=no-name-in-module

def execute(): #pylint: disable=unused-variable

  # Find out which algorithm the user has requested
  alg = algorithm.get(app.ARGS.algorithm)

  # Check for prior existence of output files, and grab any input files, used by the particular algorithm
  if app.ARGS.voxels:
    app.check_output_path(app.ARGS.voxels)
  alg.check_output_paths()

  # Sanitise some inputs, and get ready for data import
  if app.ARGS.lmax:
    try:
      lmax = [ int(x) for x in app.ARGS.lmax.split(',') ]
      if any(lmax_value%2 for lmax_value in lmax):
        raise MRtrixError('Value of lmax must be even')
    except ValueError as exception:
      raise MRtrixError('Parameter lmax must be a number') from exception
    if alg.NEEDS_SINGLE_SHELL and not len(lmax) == 1:
      raise MRtrixError('Can only specify a single lmax value for single-shell algorithms')
  shells_option = ''
  if app.ARGS.shells:
    try:
      shells_values = [ int(round(float(x))) for x in app.ARGS.shells.split(',') ]
    except ValueError as exception:
      raise MRtrixError('-shells option should provide a comma-separated list of b-values') from exception
    if alg.NEEDS_SINGLE_SHELL and not len(shells_values) == 1:
      raise MRtrixError('Can only specify a single b-value shell for single-shell algorithms')
    shells_option = ' -shells ' + app.ARGS.shells
  singleshell_option = ''
  if alg.NEEDS_SINGLE_SHELL:
    singleshell_option = ' -singleshell -no_bzero'

  grad_import_option = app.read_dwgrad_import_options()
  if not grad_import_option and 'dw_scheme' not in image.Header(path.from_user(app.ARGS.input, False)).keyval():
    raise MRtrixError('Script requires diffusion gradient table: either in image header, or using -grad / -fslgrad option')

  app.make_scratch_dir()

  # Get standard input data into the scratch directory
  if alg.NEEDS_SINGLE_SHELL or shells_option:
    app.console('Importing DWI data (' + path.from_user(app.ARGS.input) + ') and selecting b-values...')
    run.command('mrconvert ' + path.from_user(app.ARGS.input) + ' - -strides 0,0,0,1' + grad_import_option + ' | dwiextract - ' + path.to_scratch('dwi.mif') + shells_option + singleshell_option, show=False)
  else: # Don't discard b=0 in multi-shell algorithms
    app.console('Importing DWI data (' + path.from_user(app.ARGS.input) + ')...')
    run.command('mrconvert ' + path.from_user(app.ARGS.input) + ' ' + path.to_scratch('dwi.mif') + ' -strides 0,0,0,1' + grad_import_option, show=False)
  if app.ARGS.mask:
    app.console('Importing mask (' + path.from_user(app.ARGS.mask) + ')...')
    run.command('mrconvert ' + path.from_user(app.ARGS.mask) + ' ' + path.to_scratch('mask.mif') + ' -datatype bit', show=False)

  alg.get_inputs()

  app.goto_scratch_dir()

  if alg.SUPPORTS_MASK:
    if app.ARGS.mask:
      # Check that the brain mask is appropriate
      mask_header = image.Header('mask.mif')
      if mask_header.size()[:3] != image.Header('dwi.mif').size()[:3]:
        raise MRtrixError('Dimensions of provided mask image do not match DWI')
      if not (len(mask_header.size()) == 3 or (len(mask_header.size()) == 4 and mask_header.size()[3] == 1)):
        raise MRtrixError('Provided mask image needs to be a 3D image')
    else:
      app.console('Computing brain mask (dwi2mask)...')
      run.command('dwi2mask ' + CONFIG['Dwi2maskAlgorithm'] + ' dwi.mif mask.mif', show=False)

    if not image.statistics('mask.mif', mask='mask.mif').count:
      raise MRtrixError(('Provided' if app.ARGS.mask else 'Generated') + ' mask image does not contain any voxels')

  # From here, the script splits depending on what estimation algorithm is being used
  alg.execute()
