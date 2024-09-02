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

import copy, numbers, os, shutil, sys



def usage(cmdline): #pylint: disable=unused-variable
  from mrtrix3 import app, version #pylint: disable=no-name-in-module, import-outside-toplevel
  cmdline.set_author('Robert E. Smith (robert.smith@florey.edu.au)')
  cmdline.set_synopsis('Check the orientation of the diffusion gradient table')
  cmdline.add_description('Note that the corrected gradient table can be output using the -export_grad_{mrtrix,fsl} option.')
  cmdline.add_description('Note that if the -mask command-line option is not specified,'
                          ' the MRtrix3 command dwi2mask will automatically be called'
                          ' to derive a binary mask image to be used for streamline seeding'
                          ' and to constrain streamline propagation.'
                          ' More information on mask derivation from DWI data can be found at the following link: \n'
                          f'https://mrtrix.readthedocs.io/en/{version.TAG}/dwi_preprocessing/masking.html')
  cmdline.add_citation('Jeurissen, B.; Leemans, A.; Sijbers, J. '
                       'Automated correction of improperly rotated diffusion gradient orientations in diffusion weighted MRI. '
                       'Medical Image Analysis, 2014, 18(7), 953-962')
  cmdline.add_argument('input',
                       type=app.Parser.ImageIn(),
                       help='The input DWI series to be checked')
  cmdline.add_argument('-mask',
                       type=app.Parser.ImageIn(),
                       help='Provide a mask image within which to seed & constrain tracking')
  cmdline.add_argument('-number',
                       type=app.Parser.Int(1),
                       metavar='count',
                       default=10000,
                       help='Set the number of tracks to generate for each test')

  app.add_dwgrad_export_options(cmdline)
  app.add_dwgrad_import_options(cmdline)




def execute(): #pylint: disable=unused-variable
  from mrtrix3 import CONFIG, MRtrixError #pylint: disable=no-name-in-module, import-outside-toplevel
  from mrtrix3 import app, image, matrix, run #pylint: disable=no-name-in-module, import-outside-toplevel

  image_dimensions = image.Header(app.ARGS.input).size()
  if len(image_dimensions) != 4:
    raise MRtrixError('Input image must be a 4D image')
  if min(image_dimensions) == 1:
    raise MRtrixError('Cannot perform tractography on an image with a unity dimension')
  num_volumes = image_dimensions[3]

  app.activate_scratch_dir()
  # Make sure the image data can be memory-mapped
  run.command(f'mrconvert {app.ARGS.input} data.mif -strides 0,0,0,1 -datatype float32',
              preserve_pipes=True)
  if app.ARGS.grad:
    shutil.copy(app.ARGS.grad, 'grad.b')
  elif app.ARGS.fslgrad:
    shutil.copy(app.ARGS.fslgrad[0], 'bvecs')
    shutil.copy(app.ARGS.fslgrad[1], 'bvals')
  if app.ARGS.mask:
    run.command(['mrconvert', app.ARGS.mask, 'mask.mif', '-datatype', 'bit'],
                preserve_pipes=True)

  # Make sure we have gradient table stored externally to header in both MRtrix and FSL formats
  if not os.path.isfile('grad.b'):
    if os.path.isfile('bvecs'):
      run.command('mrinfo data.mif -fslgrad bvecs bvals -export_grad_mrtrix grad.b')
    else:
      run.command('mrinfo data.mif -export_grad_mrtrix grad.b')

  if not os.path.isfile('bvecs'):
    if os.path.isfile('grad.b'):
      run.command('mrinfo data.mif -grad grad.b -export_grad_fsl bvecs bvals')
    else:
      run.command('mrinfo data.mif -export_grad_fsl bvecs bvals')

  # Import both of these into local memory
  grad_mrtrix = matrix.load_matrix('grad.b')
  grad_fsl = matrix.load_matrix('bvecs')
  # Is our gradient table of the correct length?
  if not len(grad_mrtrix) == num_volumes:
    raise MRtrixError('Number of entries in gradient table does not match number of DWI volumes')
  if not len(grad_fsl) == 3 or not len(grad_fsl[0]) == num_volumes:
    raise MRtrixError('Internal error (inconsistent gradient table storage)')


  # Generate a brain mask if we weren't provided with one
  # Note that gradient table must be explicitly loaded, since there may not
  #   be one in the image header (user may be relying on -grad or -fslgrad input options)
  if not os.path.exists('mask.mif'):
    run.command(['dwi2mask', CONFIG['Dwi2maskAlgorithm'], 'data.mif', 'mask.mif', '-grad', 'grad.b'])

  # How many tracks are we going to generate?
  number_option = f' -select {app.ARGS.number}'


  #  What variations of gradient errors can we conceive?

  # Done:
  # * Has an axis been flipped? (none, 0, 1, 2)
  # * Have axes been swapped? (012 021 102 120 201 210)
  # * For both flips & swaps, it could occur in either scanner or image space...

  # To do:
  # * Have the gradients been defined with respect to image space rather than scanner space?
  # * After conversion to gradients in image space, are they _then_ defined with respect to scanner space?
  #   (should the above two be tested independently from the axis flips / permutations?)


  axis_flips = [ 'none', 0, 1, 2 ]
  axis_permutations = [ ( 0, 1, 2 ), (0, 2, 1), (1, 0, 2), (1, 2, 0), (2, 0, 1), (2, 1, 0) ]
  grad_basis = [ 'scanner', 'image' ]
  total_tests = len(axis_flips) * len(axis_permutations) * len(grad_basis)


  # List where the first element is the mean length
  lengths = [ ]

  progress = app.ProgressBar(f'Testing gradient table alterations (0 of {total_tests})', total_tests)

  for flip in axis_flips:
    for permutation in axis_permutations:
      for basis in grad_basis:

        perm = ''.join(map(str, permutation))
        suffix = f'_flip{flip}_perm{perm}_{basis}'

        if basis == 'scanner':

          grad = copy.copy(grad_mrtrix)

          # Don't do anything if there aren't any axis flips occurring (flip == 'none')
          if isinstance(flip, numbers.Number):
            multiplier = [ 1.0, 1.0, 1.0, 1.0 ]
            multiplier[flip] = -1.0
            grad = [ [ r*m for r,m in zip(row, multiplier) ] for row in grad ]

          grad = [ [ row[permutation[0]], row[permutation[1]], row[permutation[2]], row[3] ] for row in grad ]

          # Create the gradient table file
          grad_path = f'grad{suffix}.b'
          with open(grad_path, 'w', encoding='utf-8') as grad_file:
            for line in grad:
              grad_file.write (','.join([str(v) for v in line]) + '\n')

          grad_option = f' -grad {grad_path}'

        else:

          grad = copy.copy(grad_fsl)

          if isinstance(flip, numbers.Number):
            grad[flip] = [ -v for v in grad[flip] ]

          grad = [ grad[permutation[0]], grad[permutation[1]], grad[permutation[2]] ]

          grad_path = 'bvecs' + suffix
          with open(grad_path, 'w', encoding='utf-8') as bvecs_file:
            for line in grad:
              bvecs_file.write (' '.join([str(v) for v in line]) + '\n')

          grad_option = f' -fslgrad {grad_path} bvals'

        # Run the tracking experiment
        run.command(f'tckgen -algorithm tensor_det data.mif -seed_image mask.mif -mask mask.mif -minlength 0 -downsample 5 tracks{suffix}.tck '
                    f'{grad_option} {number_option}')

        # Get the mean track length
        meanlength=float(run.command(f'tckstats tracks{suffix}.tck -output mean -ignorezero').stdout)

        # Add to the database
        lengths.append([meanlength,flip,permutation,basis])

        # Increament the progress bar
        progress.increment(f'Testing gradient table alterations ({len(lengths)} of {total_tests})')

  progress.done()

  # Sort the list to find the best gradient configuration(s)
  lengths.sort()
  lengths.reverse()


  # Provide a printout of the mean streamline length of each gradient table manipulation
  sys.stderr.write('Mean length     Axis flipped    Axis permutations    Axis basis\n')
  for line in lengths:
    if isinstance(line[1], numbers.Number):
      flip_str = f'{line[1]:4d}'
    else:
      flip_str = line[1]
    length_string = '{line[0]:5.2f}'
    sys.stderr.write(f'{length_string}         {flip_str}                {line[2]}           {line[3]}\n')


  # If requested, extract what has been detected as the best gradient table, and
  #   export it in the format requested by the user
  grad_export_option = app.dwgrad_export_options()
  if grad_export_option:
    best = lengths[0]
    perm = ''.join(map(str, best[2]))
    suffix = f'_flip{best[1]}_perm{perm}_{best[3]}'
    if best[3] == 'scanner':
      grad_import_option = ['-grad', f'grad{suffix}.b']
    elif best[3] == 'image':
      grad_import_option = ['-fslgrad', f'bvecs{suffix}', 'bvals']
    else:
      assert False
    run.command(['mrinfo', 'data.mif']
                + grad_import_option
                + grad_export_option,
                force=app.FORCE_OVERWRITE)
