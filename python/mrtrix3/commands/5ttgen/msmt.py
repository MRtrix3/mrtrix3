# Copyright (c) 2008-2024 the MRtrix3 contributors.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Covered Software is provided under this License on an 'as is'
# basis, without warranty of any kind, either expressed, implied, or
# statutory, including, without limitation, warranties that the
# Covered Software is free of defects, merchantable, fit for a
# particular purpose or non-infringing.
# See the Mozilla Public License v. 2.0 for more details.
#
# For more details, see http://www.mrtrix.org/.

import os
import subprocess
from mrtrix3 import MRtrixError, app, path, run, image


def usage(base_parser, subparsers):  # pylint: disable=unused-variable
  parser = subparsers.add_parser('msmt', parents=[base_parser])
  parser.set_author('Arkiev D\'Souza (arkiev.dsouza@sydney.edu.au) & Robert E. Smith (robert.smith@florey.edu.au)')
  parser.set_synopsis('Generate a 5TT image from ODF images.')

  parser.add_argument('odf_wm', type=app.Parser.ImageIn(), help='The input white-matter ODF')
  parser.add_argument('odf_gm', type=app.Parser.ImageIn(), help='The input grey-matter ODF')
  parser.add_argument('odf_csf', type=app.Parser.ImageIn(), help='The input cerebrospinal fluid ODF')
  parser.add_argument('output', type=app.Parser.ImageOut(), help='The output 5TT image')

  options = parser.add_argument_group('Options specific to the 'msmt' algorithm')
  parser.add_argument('-mask', nargs='?', default=None, type=app.Parser.ImageIn(), help='The input binary brain mask image')


def execute(): # pylint: disable=unused-variable
  # Extract l=0 term from WM
  run.command(f'mrconvert {app.ARGS.odf_wm} -coord 3 0 -axes 0,1,2 wm_vol.mif')

  # check whether the GM and CSF ODF voxel grid match that of wm_vol.mif. If the do not match, regrid with wm_vol.mif as template
  # Load the images
  wm_vol_img = image.Image('wm_vol.mif')
  gm_odf_img = image.Image(app.ARGS.odf_gm)
  csf_odf_img = image.Image(app.ARGS.odf_csf)

  # Check GM ODF against WM ODF
  if image.match(wm_vol_img, gm_odf_img):
      app.console('Voxel grid of GM and WM ODF match')
      run.command(f'mrconvert {app.ARGS.odf_gm} gm_vol.mif')
  else:
      app.warn('GM ODF has different dimensions to WM ODF. Regridding GM ODF')
      run.command(f'mrgrid {app.ARGS.odf_gm} -template wm_vol.mif regrid gm_vol.mif')

  # Check CSF ODF against WM ODF
  if image.match(wm_vol_img, csf_odf_img):
      app.console('Voxel grid of CSF and WM ODF match')
      run.command(f'mrconvert {app.ARGS.odf_csf} csf_vol.mif')
  else:
      app.warn('CSF ODF has different dimensions to WM ODF. Regridding CSF ODF')
      run.command(f'mrgrid {app.ARGS.odf_csf} -template wm_vol.mif regrid csf_vol.mif')

  # Set negative values to zero
  run.command('mrcalc wm_vol.mif 0.0 -max wm_vol_pos.mif')
  run.command('mrcalc gm_vol.mif 0.0 -max gm_vol_pos.mif')
  run.command('mrcalc csf_vol.mif 0.0 -max csf_vol_pos.mif')

  # Normalize each volume
  run.command('mrmath wm_vol_pos.mif gm_vol_pos.mif csf_vol_pos.mif sum totalvol.mif')
  run.command('mrcalc wm_vol_pos.mif totalvol.mif -divide wm_vol_pos_norm.mif')
  run.command('mrcalc gm_vol_pos.mif totalvol.mif -divide gm_vol_pos_norm.mif')
  run.command('mrcalc csf_vol_pos.mif totalvol.mif -divide csf_vol_pos_norm.mif')

  # Create empty volume for SGM and pathology
  run.command('mrcalc wm_vol.mif inf -gt empty_vol.mif')

  # Concatenate volumes
  run.command('mrcat -datatype float32 gm_vol_pos_norm.mif empty_vol.mif wm_vol_pos_norm.mif csf_vol_pos_norm.mif empty_vol.mif fTT_dirty.mif')

  #########################
  # Tidy image by masking #
  #########################

# if mask is provided, use it. If not, create one
  if app.ARGS.mask:
    # case 1: using provided brainmask
    # check if dimensions of brainmask match 5TT_dirty. If not, regrid
    mask_img = image.Image(app.ARGS.mask)
    if image.match(mask_img, wm_vol_img):
      app.console('Mask has equal dimensions to 5TT image. No regridding of mask required')
      run.command(f'mrcalc fTT_dirty.mif {app.ARGS.mask} -mult result.mif')
    else:
      app.warn('Mask has different dimensions to 5TT image; regridding')
      run.command(f'mrgrid {app.ARGS.mask} -template wm_vol.mif regrid - | mrcalc - 0.5 -gt - | mrcalc - fTT_dirty.mif -mult result.mif')

  else:
    # case 2: Generate a mask that contains only those voxels where the sum of ODF l=0 terms exceeds 0.5/sqrt(4pi); then select the largest component and fill any holes. 
    # 0.5/sqrt(4pi)=0.1410473959
    run.command('mrcalc totalvol.mif 0.1410473959 -gt - | maskfilter - clean - | maskfilter - fill - | mrcalc - fTT_dirty.mif -mult result.mif')

  run.command(['mrconvert', 'result.mif', app.ARGS.output],
    force=app.FORCE_OVERWRITE,
    preserve_pipes=True)

  return 'result.mif'

