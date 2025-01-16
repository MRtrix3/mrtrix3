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

import json, os, shutil



# These fields are compared across headers;
#   if these values are not identical, then the expectation of equivalent b=0 image intensity
#   is erroneous, and that component of the script should not be utilised
BZERO_CONTRAST_FIELDS = ( 'EchoTime', 'RepetitionTime', 'FlipAngle' )



def usage(cmdline): #pylint: disable=unused-variable
  from mrtrix3 import app #pylint: disable=no-name-in-module, import-outside-toplevel

  cmdline.set_author('Lena Dorfschmidt (ld548@cam.ac.uk)'
                     ' and Jakub Vohryzek (jakub.vohryzek@queens.ox.ac.uk)'
                     ' and Robert E. Smith (robert.smith@florey.edu.au)')
  cmdline.set_synopsis('Concatenating multiple DWI series accounting for differential intensity scaling')
  cmdline.add_description('This script concatenates two or more 4D DWI series,'
                          ' accounting for various detrimental confounds that may affect such an operation.'
                          ' Each of those confounds are described in separate paragraphs below.')
  cmdline.add_description('There may be differences in intensity scaling between the input series.'
                          ' This intensity scaling is corrected by determining scaling factors that will make'
                          ' the overall image intensities in the b=0 volumes of each series approximately equivalent.'
                          ' This operation is only appropriate if the sequence parameters that influence the contrast'
                          ' of the b=0 image volumes are identical.')
  cmdline.add_description('Concatenation of DWI series defined on different voxel grids.'
                          ' If the voxel grids of the input DWI series are not precisely identical,'
                          ' then it may not be possible to do a simple concatenation operation.'
                          ' In this scenario the script will determine the appropriate way to combine the input series,'
                          ' ideally only manipulating header transformations and avoiding image interpolation if possible.')
  cmdline.add_argument('inputs',
                       nargs='+',
                       type=app.Parser.ImageIn(),
                       help='Multiple input diffusion MRI series')
  cmdline.add_argument('output',
                       type=app.Parser.ImageOut(),
                       help='The output image series (all DWIs concatenated)')
  cmdline.add_argument('-mask',
                       type=app.Parser.ImageIn(),
                       help='Provide a binary mask within which image intensities will be matched')
  cmdline.add_argument('-nointensity',
                       action='store_true',
                       default=None,
                       help='Do not perform intensity matching based on b=0 volumes')



def execute(): #pylint: disable=unused-variable
  from mrtrix3 import CONFIG, MRtrixError #pylint: disable=no-name-in-module, import-outside-toplevel
  from mrtrix3 import app, image, run #pylint: disable=no-name-in-module, import-outside-toplevel

  num_inputs = len(app.ARGS.inputs)
  if num_inputs < 2:
    raise MRtrixError('Script requires at least two input image series')

  # check input data
  def check_header(header):
    if len(header.size()) > 4:
      raise MRtrixError(f'Image "{header.name()}" contains more than 4 dimensions')
    if not 'dw_scheme' in header.keyval():
      raise MRtrixError(f'Image "{header.name()}" does not contain a gradient table')
    dw_scheme = header.keyval()['dw_scheme']
    try:
      if isinstance(dw_scheme[0], list):
        num_grad_lines = len(dw_scheme)
      elif (isinstance(dw_scheme[0], (int, float))) and len(dw_scheme) >= 4:
        num_grad_lines = 1
      else:
        raise MRtrixError
    except (IndexError, MRtrixError):
      raise MRtrixError(f'Image "{header.name()}" contains gradient table of unknown format') # pylint: disable=raise-missing-from
    if len(header.size()) == 4:
      num_volumes = header.size()[3]
      if num_grad_lines != num_volumes:
        raise MRtrixError(f'Number of lines in gradient table for image "{header.name()}" ({num_grad_lines}) '
                          f'does not match number of volumes ({num_volumes})')
    elif not (num_grad_lines == 1 and len(dw_scheme) >= 4 and dw_scheme[3] <= float(CONFIG.get('BZeroThreshold', 10.0))):
      raise MRtrixError(f'Image "{header.name()}" is 3D, and cannot be validated as a b=0 volume')

  first_header = image.Header(app.ARGS.inputs[0])
  check_header(first_header)

  do_intensity_matching = not app.ARGS.nointensity
  bzero_contrast_mismatch = False
  # Determine the most suitable mechanism by which to combine the data across series,
  #   as we want to minimise unnecessary manipulation of data:
  #   - 'Equal': Voxel grids match exactly, can do a straight concatenation
  #     - If user specified a mask, can use it for all series without modification
  #   - 'Rigidbody': Voxel spacing and sizes match exactly, but header transforms are different
  #     Will need to apply linear transforms to an average header in order to rotate gradients
  #     - If user specified a mask, can't proceed
  #   - 'Inequal': Voxel grids do not match in any reasonable way;
  #     will need to do an interpolation to get the data onto a common grid
  #     - If user specified a mask, can't proceed
  voxel_grid_matching = 'equal'
  for filename in app.ARGS.inputs[1:]:
    this_header = image.Header(filename)
    check_header(this_header)
    if max(abs(a-b) for a, b in zip(this_header.spacing()[0:3], first_header.spacing()[0:3])) > 1e-5 \
          or this_header.size()[0:3] != first_header.size()[0:3]:
      voxel_grid_matching = 'inequal'
    elif voxel_grid_matching == 'equal' and not image.match(this_header, first_header, up_to_dim=3):
      voxel_grid_matching = 'rigidbody'
    for field_name in BZERO_CONTRAST_FIELDS:
      first_value = first_header.keyval().get(field_name)
      this_value = this_header.keyval().get(field_name)
      if first_value and this_value and first_value != this_value:
        bzero_contrast_mismatch = True
  if voxel_grid_matching == 'rigidbody':
    app.console('Rigid-body difference in header transforms found between input series;')
    app.console('  series will need to be rigid-body transformed to average header space')
  elif voxel_grid_matching == 'inequal':
    app.console('Incompatible voxel grids between input series;')
    app.console('  data will be resampled onto a new average header space')
  else:
    assert voxel_grid_matching == 'equal'
    app.console('Input images are already on the same voxel grid;')
    app.console('  series will be concatenated with no transformation applied')
  if bzero_contrast_mismatch:
    app.warn('Mismatched protocol acquisition parameters detected between input images;')
    app.warn('  since b=0 images may have different intensities / contrasts, intensity matching across series will not be performed')
    do_intensity_matching = False
  if app.ARGS.mask:
    if voxel_grid_matching != 'equal':
      raise MRtrixError('Input mask cannot be unambiguously interpreted due to differences in input image headers')
    mask_header = image.Header(app.ARGS.mask)
    if mask_header.size()[0:3] != first_header.size()[0:3]:
      raise MRtrixError(f'Spatial dimensions of mask image "{app.ARGS.mask}" do not match those of first image "{first_header.name()}"')

  # import data to scratch directory
  app.activate_scratch_dir()
  for index, filename in enumerate(app.ARGS.inputs):
    run.command(['mrconvert', filename, f'{index}in.mif'],
                preserve_pipes=True)
  if app.ARGS.mask:
    run.command(['mrconvert', app.ARGS.mask, 'mask.mif', '-datatype', 'bit'],
                preserve_pipes=True)

  if do_intensity_matching:
    # extract b=0 volumes within each input series
    for index in range(0, num_inputs):
      infile = f'{index}in.mif'
      outfile = f'{index}b0.mif'
      if len(image.Header(infile).size()) > 3:
        run.command(f'dwiextract {infile} {outfile} -bzero')
      else:
        run.function(shutil.copyfile, infile, outfile)

    mask_option = ['-mask_input', 'mask.mif', '-mask_target', 'mask.mif'] if app.ARGS.mask else []

    # for all but the first image series:
    #   - find multiplicative factor to match b=0 images to those of the first image
    #   - apply multiplicative factor to whole image series
    # It would be better to not preferentially treat one of the inputs differently to any other:
    #   - compare all inputs to all other inputs
    #   - determine one single appropriate scaling factor for each image based on all results
    #       can't do a straight geometric average: e.g. if run for 2 images, each would map to
    #         the the input intensoty of the other image, and so the two corrected images would not match
    #       should be some mathematical theorem providing the optimal scaling factor for each image
    #         based on the resulting matrix of optimal scaling factors
    rescaled_filelist = [ '0in.mif' ]
    for index in range(1, num_inputs):
      stderr_text = run.command(['mrhistmatch', 'scale', f'{index}b0.mif', '0b0.mif', f'{index}rescaledb0.mif']
                                + mask_option).stderr
      scaling_factor = None
      for line in stderr_text.splitlines():
        if 'Estimated scale factor is' in line:
          try:
            scaling_factor = float(line.split()[-1])
          except ValueError as exception:
            raise MRtrixError('Unable to convert scaling factor from mrhistmatch output to floating-point number') from exception
          break
      if scaling_factor is None:
        raise MRtrixError('Unable to extract scaling factor from mrhistmatch output')
      filename = f'{index}rescaled.mif'
      run.command(f'mrcalc {index}in.mif {scaling_factor} -mult {filename}')
      rescaled_filelist.append(filename)
  else:
    rescaled_filelist = [f'{index}in.mif' for index in range(0, len(app.ARGS.inputs))]

  if voxel_grid_matching == 'equal':
    transformed_filelist = rescaled_filelist
  elif voxel_grid_matching in ('rigidbody', 'inequal'):
    run.command(['mraverageheader', rescaled_filelist, 'average_header.mif',
                 '-spacing', ('mean_nearest' if voxel_grid_matching == 'rigidbody' else 'min_nearest')])
    transformed_filelist = []
    for item in rescaled_filelist:
      transformname = f'{os.path.splitext(item)[0]}_to_avgheader.txt'
      imagename = f'{os.path.splitext(item)[0]}_transformed.mif'
      run.command(['transformcalc', item, 'average_header.mif', 'header', transformname])
      # Ensure that no non-rigid distortions of image data are applied
      #   in the process of moving it toward the average header
      if voxel_grid_matching == 'inequal':
        newtransformname = f'{os.path.splitext(transformname)[0]}_rigid.txt'
        run.command(['transformcalc', transformname, 'rigid', newtransformname])
        transformname = newtransformname
      run.command(['mrtransform', item, imagename] \
                  + ['-linear', transformname] \
                  + ['-reorient_fod', 'no'] \
                  + (['-template', 'average_header.mif', '-interp', 'sinc'] \
                     if voxel_grid_matching == 'inequal' \
                      else []))
      transformed_filelist.append(imagename)
  else:
    assert False

  # concatenate all series together
  run.command(['mrcat', transformed_filelist, '-', '-axis', '3', '|',
              'mrconvert', '-', 'result.mif', '-json_export', 'result_init.json', '-strides', '0,0,0,1'])

  # remove current contents of command_history, since there's no sensible
  #   way to choose from which input image the contents should be taken;
  #   we do however want to keep other contents of keyval (e.g. gradient table)
  with open('result_init.json', 'r', encoding='utf-8') as input_json_file:
    keyval = json.load(input_json_file)
  keyval.pop('command_history', None)
  # If we had to perform interpolation, no longer guaranteed correspondence between
  #   slice position and relative acquisition time
  if voxel_grid_matching == 'inequal':
    keyval.pop('SliceTiming', None)
  with open('result_final.json', 'w', encoding='utf-8') as output_json_file:
    json.dump(keyval, output_json_file)

  run.command(['mrconvert', 'result.mif', app.ARGS.output],
              mrconvert_keyval='result_final.json',
              force=app.FORCE_OVERWRITE)
