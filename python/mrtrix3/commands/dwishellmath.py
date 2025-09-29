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

SUPPORTED_OPS = ['mean', 'median', 'sum', 'product', 'rms', 'norm', 'var', 'std', 'min', 'max', 'absmax', 'magmax']


def usage(cmdline): #pylint: disable=unused-variable
  from mrtrix3 import app #pylint: disable=no-name-in-module, import-outside-toplevel
  cmdline.set_author('Daan Christiaens (daan.christiaens@kcl.ac.uk)')
  cmdline.set_synopsis('Apply an mrmath operation to each b-value shell in a DWI series')
  cmdline.add_description('The output of this command is a 4D image,'
                          ' where each volume corresponds to a b-value shell'
                          ' (in order of increasing b-value),'
                          ' and the intensities within each volume correspond to the chosen statistic'
                          ' having been computed from across the DWI volumes belonging to that b-value shell.')
  cmdline.add_argument('input',
                       type=app.Parser.ImageIn(),
                       help='The input diffusion MRI series')
  cmdline.add_argument('operation',
                       choices=SUPPORTED_OPS,
                       help='The operation to be applied to each shell;'
                            ' this must be one of the following: '
                            + ', '.join(SUPPORTED_OPS))
  cmdline.add_argument('output',
                       type=app.Parser.ImageOut(),
                       help='The output image series')
  cmdline.add_example_usage('To compute the mean diffusion-weighted signal in each b-value shell',
                            'dwishellmath dwi.mif mean shellmeans.mif')
  app.add_dwgrad_import_options(cmdline)


def execute(): #pylint: disable=unused-variable
  from mrtrix3 import MRtrixError #pylint: disable=no-name-in-module, import-outside-toplevel
  from mrtrix3 import app, image, run #pylint: disable=no-name-in-module, import-outside-toplevel

  # check inputs and outputs
  dwi_header = image.Header(app.ARGS.input)
  if len(dwi_header.size()) != 4:
    raise MRtrixError('Input image must be a 4D image')
  gradimport = app.dwgrad_import_options()
  if not gradimport and 'dw_scheme' not in dwi_header.keyval():
    raise MRtrixError('No diffusion gradient table provided, and none present in image header')
  # import data and gradient table
  app.activate_scratch_dir()
  run.command(['mrconvert', app.ARGS.input, 'in.mif',
               '-strides', '0,0,0,1']
              + gradimport,
              preserve_pipes=True)
  # run per-shell operations
  files = []
  for index, bvalue in enumerate(image.mrinfo('in.mif', 'shell_bvalues').split()):
    filename = f'shell-{index:02d}.mif'
    run.command(f'dwiextract -shells {bvalue} in.mif - | '
                f'mrmath -axis 3 - {app.ARGS.operation} {filename}')
    files.append(filename)
  if len(files) > 1:
    # concatenate to output file
    run.command(['mrcat', '-axis', '3', files, 'out.mif'])
    run.command(['mrconvert', 'out.mif', app.ARGS.output],
                mrconvert_keyval=app.ARGS.input,
                force=app.FORCE_OVERWRITE,
                preserve_pipes=True)
  else:
    # make a 4D image with one volume
    app.warn('Only one unique b-value present in DWI data; '
             'command mrmath with -axis 3 option may be preferable')
    run.command(['mrconvert', files[0], app.ARGS.output, '-axes', '0,1,2,-1'],
                mrconvert_keyval=app.ARGS.input,
                force=app.FORCE_OVERWRITE,
                preserve_pipes=True)
