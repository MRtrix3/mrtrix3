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

import os
import subprocess
from mrtrix3 import MRtrixError
from mrtrix3 import app, path, run


def usage(base_parser, subparsers):  # pylint: disable=unused-variable
  parser = subparsers.add_parser('msmt', parents=[base_parser])
  parser.set_author("Arkiev D'Souza (arkiev.dsouza@sydney.edu.au) & Robert E. Smith (robert.smith@florey.edu.au)")
  parser.set_synopsis('Generate a 5TT image from ODF images.')

  parser.add_argument('odf_wm', type=app.Parser.ImageIn(), help='The input white-matter ODF')
  parser.add_argument('odf_gm', type=app.Parser.ImageIn(), help='The input grey-matter ODF')
  parser.add_argument('odf_csf', type=app.Parser.ImageIn(), help='The input cerebrospinal fluid ODF')
  parser.add_argument('fTT_image', type=app.Parser.ImageOut(), help='The output 5TT image')

  options = parser.add_argument_group('Options specific to the "odf" algorithm')
  parser.add_argument('-mask', nargs='?', default=None, type=app.Parser.ImageIn(), help='The input binary brain mask image')


def execute(): # pylint: disable=unused-variable
  # Extract l=0 term from WM
  run.command('mrconvert {app.ARGS.odf_wm} -coord 3 0 -axes 0,1,2 wm_vol.mif')

  # check whether the GM and CSF ODF voxel grid match that of wm_vol.mif. If the do not match, regrid with wm_vol.mif as template
  command_size_WModf = 'mrinfo -spacing wm_vol.mif'
  spacing_size_WModf = subprocess.check_output(command_size_WModf, shell=True).decode('utf-8')
  command_size_GModf = 'mrinfo -spacing {app.ARGS.odf_gm}'
  spacing_size_GModf = subprocess.check_output(command_size_GModf, shell=True).decode('utf-8')
  command_size_CSFodf = 'mrinfo -spacing {app.ARGS.odf_gm}'
  spacing_size_CSFodf = subprocess.check_output(command_size_CSFodf, shell=True).decode('utf-8')

  if spacing_size_WModf == spacing_size_GModf:
    print("voxel grid of GM and WM ODF match")
    run.command('mrconvert {app.ARGS.odf_gm} gm_vol.mif')
  else:
    print("WARNING: GM ODF has different dimeonsions to WM ODF. Regridding GM ODF")
    run.command('mrgrid {app.ARGS.odf_gm} -template wm_vol.mif regrid gm_vol.mif')

  if spacing_size_WModf == spacing_size_CSFodf:
    print("voxel grid of CSF and WM ODF match")
    run.command('mrconvert {app.ARGS.odf_csf} csf_vol.mif')
  else:
    print("WARNING: CSF ODF has different dimeonsions to WM ODF. Regridding CSF ODF")
    run.command('mrgrid {app.ARGS.odf_csf} -template wm_vol.mif regrid csf_vol.mif')

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
    command_fTTdirty = 'mrinfo -spacing fTT_dirty.mif'
    spacing_fTTdirty = subprocess.check_output(command_fTTdirty, shell=True).decode('utf-8')

    command_mask = 'mrinfo -spacing {app.ARGS.mask}'
    spacing_mask = subprocess.check_output(command_mask, shell=True).decode('utf-8')

      if spacing_fTTdirty == spacing_mask:
        print("mask has equal dimensions to 5TT image. No regridding of mask required")
        run.command('mrcalc fTT_dirty.mif {app.ARGS.mask} -mult result.mif')

      else:
        print("WARNING: mask has different dimeonsions to 5TT image. Regridding mask")
        run.command('mrgrid {app.ARGS.mask} -template wm_vol.mif regrid - | mrcalc - 0.5 -gt - | mrcalc - fTT_dirty.mif -mult result.mif')


  elif not app.ARGS.mask:
    # case 2: Generate a mask that contains only those voxels where the sum of ODF l=0 terms exceeds 0.5/sqrt(4pi); then select the largest component and fill any holes. 
    # 0.5/sqrt(4pi)=0.1410473959
    run.command('mrcalc totalvol.mif 0.1410473959 -gt - | maskfilter - clean - | maskfilter - fill - | mrcalc - fTT_dirty.mif -mult result.mif')

  run.command(['mrconvert', 'result.mif', app.ARGS.fTT_image],
    force=app.FORCE_OVERWRITE,
    preserve_pipes=True)

  return 'result.mif'

