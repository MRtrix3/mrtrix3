# Copyright (c) 2008-2024 the MRtrix3 contributors.
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

from mrtrix3 import app, image, run


def usage(base_parser, subparsers):  # pylint: disable=unused-variable
  parser = subparsers.add_parser('msmt', parents=[base_parser])
  parser.set_author('Arkiev D\'Souza (arkiev.dsouza@sydney.edu.au) & Robert E. Smith (robert.smith@florey.edu.au)')
  parser.set_synopsis('Generate a 5TT image from ODF images')
  parser.add_description('If the user does not manually provide a brain mask using the -mask option,'
                         ' the command will automatically determine a mask for the output 5TT image;'
                         ' this is necessary to conform to the expectations of the 5TT format.'
                         ' This mask will be computed based on where the sum of the ODFs exceeds 50%'
                         ' of what is expected from a voxel that contains a DWI signal'
                         ' that exactly matches one of the response functions.')

  parser.add_argument('odf_wm', type=app.Parser.ImageIn(), help='The input white matter ODF')
  parser.add_argument('odf_gm', type=app.Parser.ImageIn(), help='The input grey matter ODF')
  parser.add_argument('odf_csf', type=app.Parser.ImageIn(), help='The input cerebrospinal fluid ODF')
  parser.add_argument('output', type=app.Parser.ImageOut(), help='The output 5TT image')

  options = parser.add_argument_group('Options specific to the \'msmt\' algorithm')
  options.add_argument('-mask', type=app.Parser.ImageIn(), help='An input binary brain mask image')


def execute(): # pylint: disable=unused-variable

  target_voxel_grid = image.Header(app.ARGS.odf_wm)

  class Tissue:
    def __init__(self, inpath, name, expect_anisotropic):
      self.name = name
      header = image.Header(inpath)
      do_regrid = not image.match(header, target_voxel_grid, up_to_dim=3)
      if do_regrid:
        app.warn(f'ODF image "{inpath}" not defined on same voxel grid as WM ODF image;'
                 ' will have to resample')
      else:
        app.debug(f'{name} ODF image already on target voxel grid')
      is_anisotropic = len(header.size()) > 3 and header.size()[3] > 1
      if is_anisotropic != expect_anisotropic:
        app.warn(f'Received {"anisotropic" if is_anisotropic else "isotropic"} ODF for {name}'
                 f' but expected {"anisotropic" if expect_anisotropic else "isotropic"};'
                 ' check order of input ODF images if this was not intentional')
      self.lzeropath = f'{name}_lzero.mif'
      command = ['mrgrid', inpath, 'regrid', '-template', app.ARGS.odf_wm, '-', '|'] \
                if do_regrid                                                         \
                else []
      if is_anisotropic:
        command.extend(['mrconvert', '-' if command else inpath, '-coord', '3', '0', '-axes', '0,1,2', '-', '|'])
      command.extend(['mrcalc', '-' if command else inpath, '0.0', '-max', self.lzeropath])
      run.command(command)
      self.normpath = None

    def normalise(self, volsum_image):
      assert self.normpath is None
      self.normpath = f'{self.name}_fraction.mif'
      run.command(f'mrcalc {self.lzeropath} {volsum_image} -divide {self.normpath}')

  # End definitiion of class Tissues

  tissues = [Tissue(app.ARGS.odf_wm,  'WM',  True),
             Tissue(app.ARGS.odf_gm,  'GM',  False),
             Tissue(app.ARGS.odf_csf, 'CSF', False)]

  # Compute volume sum in each voxel
  volsum_image = 'totalvol.mif'
  run.command(['mrmath', [item.lzeropath for item in tissues], 'sum', volsum_image])
  # Normalise to tissue fractions
  for tissue in tissues:
    tissue.normalise(volsum_image)

  # Create empty volume for SGM and pathology
  empty_volume = 'empty_vol.mif'
  run.command(f'mrcalc {volsum_image} inf -gt {empty_volume}')

  # Concatenate volumes
  result_unmasked = '5TT_unmasked.mif'
  ####################################################################################################################
  # 5TT volumes:          Cortical GM      Sub-cortical GM          WM                   CSF             Pathology   #
  ####################################################################################################################
  run.command(f'mrcat {tissues[1].normpath} {empty_volume} {tissues[0].normpath} {tissues[2].normpath} {empty_volume}'
              f' {result_unmasked} -datatype float32')

  # Tidy image by masking
  result_masked = '5TT_masked.mif'
  # If mask is provided, use it; if not, create one
  if app.ARGS.mask:
    # Case 1: Using provided brainmask
    # Check if brainmask is on same voxel grid; if not, regrid
    mask_header = image.Header(app.ARGS.mask)
    if image.match(target_voxel_grid, mask_header, up_to_dim=3):
      app.debug('Mask matches ODF voxel grid; no regridding of mask required')
      run.command(f'mrcalc {result_unmasked} {app.ARGS.mask} -mult {result_masked}')
    else:
      app.warn('Mask has different voxel grid to WM ODF image; regridding')
      run.command(f'mrgrid {app.ARGS.mask} -template {empty_volume} regrid - |'
                  ' mrcalc - 0.5 -gt - |'
                  f' mrcalc - {result_unmasked} -mult {result_masked}')
  else:
    # Case 2: Generate a mask that contains only those voxels where the sum of ODF l=0 terms exceeds 0.5/sqrt(4pi);
    #   then select the largest component and fill any holes.
    # 0.5/sqrt(4pi) = 0.1410473959
    app.console('No 5TT mask provided; generating from input ODFs')
    run.command(f'mrcalc {volsum_image} 0.1410473959 -gt - |'
                ' maskfilter - clean - |'
                ' maskfilter - fill - |'
                f' mrcalc - {result_unmasked} -mult {result_masked}')

  run.command(['mrconvert', result_masked, app.ARGS.output],
    force=app.FORCE_OVERWRITE,
    preserve_pipes=True)

  return result_masked
