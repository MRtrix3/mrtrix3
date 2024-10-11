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
    parser = subparsers.add_parser('odf', parents=[base_parser])
    parser.set_author("Arkiev D'Souza (arkiev.dsouza@sydney.edu.au) & Robert E. Smith (robert.smith@florey.edu.au)")
    parser.set_synopsis('Generate a 5TT image from ODF images.')
    
    parser.add_argument('odf_wm', type=app.Parser.ImageIn(), help='The input white-matter ODF')
    parser.add_argument('odf_gm', type=app.Parser.ImageIn(), help='The input grey-matter ODF')
    parser.add_argument('odf_csf', type=app.Parser.ImageIn(), help='The input cerebrospinal fluid ODF')
    parser.add_argument('mask_image', type=app.Parser.ImageIn(), help='The input binary brain mask image')
    parser.add_argument('fTT_image', type=app.Parser.ImageOut(), help='The output 5TT image')

    options = parser.add_argument_group('Options specific to the "odf" algorithm')
    options.add_argument('-sgm', nargs='?', default=None, type=app.Parser.ImageIn(), help='Provide a mask of voxels that should be set to sub-cortical grey matter')
    options.add_argument('-pathology', nargs='?', default=None, type=app.Parser.ImageIn(), help='Provide a mask of voxels that should be set to pathological tissue')


def execute(): # pylint: disable=unused-variable

    # Extract l=0 term from WM
    run.command(f'mrconvert {app.ARGS.odf_wm} -coord 3 0 -axes 0,1,2 wm_vol.mif')

    # Set negative values to zero
    run.command('mrcalc wm_vol.mif 0.0 -max wm_vol_pos.mif')
    run.command(f'mrcalc {app.ARGS.odf_gm} 0.0 -max gm_vol_pos.mif')
    run.command(f'mrcalc {app.ARGS.odf_csf} 0.0 -max csf_vol_pos.mif')

    # Normalize each volume
    run.command(f'mrmath wm_vol_pos.mif gm_vol_pos.mif csf_vol_pos.mif sum totalvol.mif')
    run.command(f'mrcalc wm_vol_pos.mif totalvol.mif -divide wm_vol_pos_norm.mif')
    run.command(f'mrcalc gm_vol_pos.mif totalvol.mif -divide gm_vol_pos_norm.mif')
    run.command(f'mrcalc csf_vol_pos.mif totalvol.mif -divide csf_vol_pos_norm.mif')

    # Create empty volume for SGM and pathology
    run.command(f'mrcalc wm_vol.mif inf -gt empty_vol.mif')

    # Concatenate volumes
    run.command(f'mrcat -datatype float32 gm_vol_pos_norm.mif empty_vol.mif wm_vol_pos_norm.mif csf_vol_pos_norm.mif empty_vol.mif fTT_dirty.mif')

    # Tidy image by masking
    run.command(f'mrgrid', app.ARGS.mask_image, '-template wm_vol.mif -interp nearest regrid mask_regridded.mif')
    run.command(f'mrcalc fTT_dirty.mif mask_regridded.mif -mult fTT_image_default.mif')

    # Finalizing output based on optional SGM and PATH
    if not app.ARGS.sgm and not app.ARGS.pathology:
        run.command(f'mrconvert fTT_image_default.mif result.mif')
    elif app.ARGS.sgm and not app.ARGS.pathology:
        run.command(f'mrgrid', app.ARGS.sgm, '-template wm_vol.mif regrid sgm_regridded.mif')
        run.command(f'5ttedit fTT_image_default.mif -sgm sgm_regridded.mif result.mif')
    elif not app.ARGS.sgm and app.ARGS.pathology:
        run.command(f'mrgrid', app.ARGS.pathology, '-template wm_vol.mif regrid path_regridded.mif')
        run.command(f'5ttedit fTT_image_default.mif -path path_regridded.mif result.mif')
    elif app.ARGS.sgm and app.ARGS.pathology:
        run.command(f'mrgrid', app.ARGS.sgm, '-template wm_vol.mif regrid sgm_regridded.mif')
        run.command(f'mrgrid', app.ARGS.pathology, '-template wm_vol.mif regrid path_regridded.mif')
        run.command(f'5ttedit fTT_image_default.mif -sgm sgm_regridded.mif -path path_regridded.mif result.mif')

    run.command(['mrconvert', 'result.mif', app.ARGS.fTT_image],
              force=app.FORCE_OVERWRITE,
              preserve_pipes=True)

  return 'result.mif'

