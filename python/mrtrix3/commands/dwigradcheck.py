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

import copy
import os
import shutil
import sys



FLIPS = (None, 0, 1, 2)
PERMUTATIONS = ((0,1,2), (0,2,1), (1,0,2), (1,2,0), (2,0,1), (2,1,0))
SHUFFLING_BASES = ('none', 'real', 'bvec')
# Names indicate that the gradient table *should have* been stored as the *latter* convention,
#   but the user has it *erroneously stored* as the *former* convention;
#   therefore, from a processing perspective,
#   code must take the data in the former format and manually transpose into the latter
# Note that this transposition could occur before or after shuffling,
#   depending on the basis in which the shuffling is performed
TRANSPOSITIONS = (None, 'real2bvec', 'bvec2real')

DEFAULT_NUMBER = 10000
DEFAULT_THRESHOLD = 0.95



def usage(cmdline): #pylint: disable=unused-variable
  from mrtrix3 import app, version #pylint: disable=no-name-in-module, import-outside-toplevel
  cmdline.set_author('Robert E. Smith (robert.smith@florey.edu.au)')
  cmdline.set_synopsis('Check the orientation of the diffusion gradient table')
  cmdline.add_description('Note that the corrected gradient table can be output using the -export_grad_{mrtrix,fsl} option.')
  cmdline.add_description('In addition to testing for potential permutations and flips of axes in the gradient table'
                          ' (which may have occurred within either the real / scanner space or bvec representations of such),'
                          ' the command also considers the prospect that the gradient table contents may have been'
                          ' computed / provided in real / scanner space but erroneously interpreted as "bvec" contents'
                          ' or vice-versa.'
                          ' To address this,'
                          ' the command additionally considers possible transpositions between these two representations.'
                          ' Where there is possibly a combination of both transposition and axis shuffles,'
                          ' that transposition will be applied either before or after the axis shuffles as necessary.'
                          ' If it is known that no such transposition has occurred,'
                          ' or if there is very close correspondence between image and real / scanner space axes,'
                          ' the search space of the command can be reduced using the -notranspose option.')
  cmdline.add_description('Note that if the -mask command-line option is not specified,'
                          ' the MRtrix3 command dwi2mask will automatically be called'
                          ' to derive a binary mask image to be used for streamline seeding'
                          ' and to constrain streamline propagation.'
                          ' More information on mask derivation from DWI data can be found at the following link: \n'
                          f'https://mrtrix.readthedocs.io/en/{version.TAG}/dwi_preprocessing/masking.html')
  cmdline.add_citation('Jeurissen, B.; Leemans, A.; Sijbers, J.'
                       ' Automated correction of improperly rotated diffusion gradient orientations in diffusion weighted MRI.'
                       ' Medical Image Analysis, 2014, 18(7), 953-962')
  cmdline.add_argument('input',
                       type=app.Parser.ImageIn(),
                       help='The input DWI series to be checked')
  cmdline.add_argument('-mask',
                       type=app.Parser.ImageIn(),
                       help='Provide a mask image within which to seed & constrain tracking')
  cmdline.add_argument('-number',
                       type=app.Parser.Int(1),
                       metavar='count',
                       default=DEFAULT_NUMBER,
                       help='Set the number of tracks to generate for each test')
  cmdline.add_argument('-threshold',
                       type=float,
                       metavar='value',
                       default=DEFAULT_THRESHOLD,
                       help='Modulate thresold on the ratio of empirical to maximal mean length to issue an error')
  cmdline.add_argument('-notranspose',
                       action='store_true',
                       help='Do not evaluate possible misattribution of gradient directions between image and scanner spaces')
  cmdline.add_argument('-shuffle',
                       choices=SHUFFLING_BASES,
                       help='Restrict the possible search space of axis shuffles;'
                            f' options are: {",".join(SHUFFLING_BASES)}'
                            ' (if omitted, shuffling of both bvec and realspace vectors will be performed)')
  cmdline.add_argument('-all',
                       action='store_true',
                       help='Print table containing all results to standard output')
  cmdline.add_argument('-out',
                       type=app.Parser.FileOut(),
                       help='Write text file with table containing all results')
  app.add_dwgrad_export_options(cmdline)
  app.add_dwgrad_import_options(cmdline)



class Variant():
  def __init__(self, flip, permutations, shuffle_basis, transpose):
    self.flip = flip
    self.permutations = permutations
    self.shuffle_basis = shuffle_basis
    self.transpose = transpose
    self.mean_length = None
  def __format__(self, fmt): #pylint: disable=too-many-return-statements
    if self.is_default():
      return 'none'
    if self.transpose == 'bvec2real':
      if self.no_shuffling():
        return 'bvec_transB2R'
      if self.shuffle_basis == 'real':
        return f'bvec_transB2R_flip{self.flip}_perm{"".join(map(str, self.permutations))}'
      assert self.shuffle_basis == 'bvec'
      return f'bvec_flip{self.flip}_perm{"".join(map(str, self.permutations))}_transB2R'
    if self.transpose == 'real2bvec':
      if self.no_shuffling():
        return 'real_transR2B'
      if self.shuffle_basis == 'real':
        return f'real_flip{self.flip}_perm{"".join(map(str, self.permutations))}_transR2B'
      assert self.shuffle_basis == 'bvec'
      return f'real_transR2B_flip{self.flip}_perm{"".join(map(str, self.permutations))}'
    if self.shuffle_basis == 'real':
      return f'real_flip{self.flip}_perm{"".join(map(str, self.permutations))}'
    assert self.shuffle_basis == 'bvec'
    return f'bvec_flip{self.flip}_perm{"".join(map(str, self.permutations))}'
  def is_default(self):
    return self.flip is None and self.permutations == (0,1,2) and self.shuffle_basis == 'none' and self.transpose is None
  def no_flip(self):
    return self.flip is None
  def no_permutations(self):
    return self.permutations == (0,1,2)
  def no_shuffling(self):
    if self.shuffle_basis == 'none':
      assert self.no_flip() and self.no_permutations()
      return True
    assert not self.no_flip() or not self.no_permutations()
    return False
  def str_transpose_preshuffle(self):
    if self.transpose == 'real2bvec' and self.shuffle_basis != 'real':
      return 'real2bvec'
    if self.transpose == 'bvec2real' and self.shuffle_basis != 'bvec':
      return 'bvec2real'
    return 'none'
  def str_transpose_postshuffle(self):
    if self.transpose == 'real2bvec' and self.shuffle_basis == 'real':
      return 'real2bvec'
    if self.transpose == 'bvec2real' and self.shuffle_basis == 'bvec':
      return 'bvec2real'
    return 'none'
  def str_shuffle(self):
    temp = [1, 2, 3]
    if self.flip is not None:
      temp[self.flip] = -temp[self.flip]
    temp = [temp[self.permutations[0]], temp[self.permutations[1]], temp[self.permutations[2]]]
    return f'{temp[0]},{temp[1]},{temp[2]}'
  def str_shuffle_pretty(self):
    if self.no_shuffling():
      return '    none     '
    return f'{self.shuffle_basis}:({self.str_shuffle()}){" " if self.flip is None else ""}'
  def out_format(self):
    if self.transpose == 'real2bvec' and self.shuffle_basis == 'real':
      return 'bvec'
    if self.transpose == 'bvec2real' and self.shuffle_basis == 'bvec':
      return 'real'
    if self.shuffle_basis == 'none':
      return 'bvec' if self.transpose == 'real2bvec' else 'real'
    return self.shuffle_basis

def sort_key(item):
  assert item.mean_length is not None
  return item.mean_length




def execute(): #pylint: disable=unused-variable
  from mrtrix3 import CONFIG, MRtrixError #pylint: disable=no-name-in-module, import-outside-toplevel
  from mrtrix3 import app, image, matrix, run #pylint: disable=no-name-in-module, import-outside-toplevel

  if app.ARGS.notranspose and app.ARGS.shuffle == 'none':
    raise MRtrixError('No candidate modified gradient tables if both -notranspose and -shuffle none are specified')

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
  grad_bvecs = matrix.load_matrix('bvecs')
  grad_bvals = matrix.load_vector('bvals')
  # Is our gradient table of the correct length?
  if not len(grad_mrtrix) == num_volumes:
    raise MRtrixError('Number of entries in gradient table does not match number of DWI volumes')
  if not len(grad_bvecs) == 3 or not len(grad_bvecs[0]) == num_volumes or not len(grad_bvals) == len(grad_bvecs[0]):
    raise MRtrixError('Internal error (inconsistent gradient table storage)')


  # Generate a brain mask if we weren't provided with one
  # Note that gradient table must be explicitly loaded, since there may not
  #   be one in the image header (user may be relying on -grad or -fslgrad input options)
  if not os.path.exists('mask.mif'):
    run.command(['dwi2mask', CONFIG['Dwi2maskAlgorithm'], 'data.mif', 'mask.mif', '-grad', 'grad.b'])

  # How many tracks are we going to generate?
  number_option = ['-select', str(app.ARGS.number)]

  variants = []
  for flip in FLIPS:
    for permutation in PERMUTATIONS:
      for shuffling_base in SHUFFLING_BASES:
        # Exclude invalid combinations
        if shuffling_base == 'none' and (flip is not None or permutation != (0,1,2)):
          continue
        if shuffling_base != 'none' and (flip is None and permutation == (0,1,2)):
          continue
        if app.ARGS.shuffle is not None and shuffling_base != app.ARGS.shuffle:
          continue
        for transposition in TRANSPOSITIONS:
          if transposition is None and shuffling_base is None:
            continue
          if transposition is not None and app.ARGS.notranspose:
            continue
          # Always omit the unmodified data here; add it at the end
          if flip is None and permutation == (0,1,2) and shuffling_base == 'none' and transposition is None:
            continue
          variants.append(Variant(flip, permutation, shuffling_base, transposition))
  # Add the "no change" variant only at the very end
  variants.append(Variant(None, (0,1,2), 'none', None))
  app.debug('Complete list of variants:')
  for variant in variants:
    app.debug(f'{variant}')

  progress = app.ProgressBar('Testing gradient table alterations' +
                             (f' (0 of {len(variants)})' if sys.stderr.isatty() else f' ({len(variants)} variants)'),
                             len(variants))
  meanlength_default = None
  for index, variant in enumerate(variants):
    # Break processing into four phases:
    # 1. Do we need to transpose the gradient table from one format to the other?
    #    (this also flags the basis in which any shuffling may be performed)
    # 2. Shuffling
    # 3. Transpose the gradient table from one format to the other
    #    (this also determines the gradient table format that will be provided to tckgen)
    # 4. Save the file to provide to tckgen in the appropriate format

    # 1. Transposition / determination of shuffling basis
    # If shuffling basis is None, still need to capture and propagate data,
    #   depending on what transposition might be occurring
    if variant.transpose == 'real2bvec' and variant.shuffle_basis != 'real':
      grad = [[row[i] for row in grad_mrtrix] for i in range(0, 3)]
      shuffling_basis = 'bvec'
    elif variant.transpose == 'bvec2real' and variant.shuffle_basis != 'bvec':
      grad = [[row[i] for row in grad_bvecs] + [grad_bvals[i]] for i in range(0, len(grad_bvecs[0]))]
      shuffling_basis = 'real'
    else:
      grad = copy.copy(grad_bvecs) if variant.shuffle_basis == 'bvec' else copy.copy(grad_mrtrix)
      shuffling_basis = variant.shuffle_basis

    # 2. Shuffling
    if shuffling_basis == 'real':
      if variant.flip is not None:
        multiplier = [ 1.0, 1.0, 1.0, 1.0 ]
        multiplier[variant.flip] = -1.0
        grad = [ [ r*m for r,m in zip(row, multiplier) ] for row in grad ]
      grad = [ [ row[variant.permutations[0]], row[variant.permutations[1]], row[variant.permutations[2]], row[3] ] for row in grad ]
    elif shuffling_basis == 'bvec':
      if variant.flip is not None:
        grad[variant.flip] = [ -v for v in grad[variant.flip] ]
      grad = [ grad[variant.permutations[0]], grad[variant.permutations[1]], grad[variant.permutations[2]] ]
    else:
      assert shuffling_basis == 'none'

    # 3. Post shuffling transposition
    save_basis = variant.out_format()
    if variant.transpose == 'real2bvec' and variant.shuffle_basis == 'real':
      grad = [[row[i] for row in grad] for i in range(0, 3)]
    elif variant.transpose == 'bvec2real' and variant.shuffle_basis == 'bvec':
      grad = [[row[i] for row in grad_bvecs] + [grad_bvals[i]] for i in range(0, len(grad[0]))]

    # 4. Save to file
    if save_basis == 'bvec':
      grad_path = f'bvecs_{variant}'
      with open(grad_path, 'w', encoding='utf-8') as bvecs_file:
        for line in grad:
          bvecs_file.write (' '.join([str(v) for v in line]) + '\n')
      grad_option = ['-fslgrad', grad_path, 'bvals']
    else:
      assert save_basis == 'real'
      grad_path = f'grad_{variant}.b'
      with open(grad_path, 'w', encoding='utf-8') as grad_file:
        for line in grad:
          grad_file.write (','.join([str(v) for v in line]) + '\n')
      grad_option = ['-grad', grad_path]

    # Run the tracking experiment
    track_file_path = f'tracks_{variant}.tck'
    run.command(['tckgen', 'data.mif',
                 '-algorithm', 'tensor_det',
                 '-seed_image', 'mask.mif',
                 '-mask', 'mask.mif',
                 '-minlength', '0',
                 '-downsample', '5',
                 track_file_path]
                + number_option
                + grad_option)

    # Get the mean track length & add to the database
    mean_length = float(run.command(['tckstats', track_file_path, '-output', 'mean', '-ignorezero']).stdout)
    variants[index].mean_length = mean_length
    app.cleanup(track_file_path)

    # Save result if this is the unmodified empirical gradient table
    if variant.is_default():
      assert meanlength_default is None
      meanlength_default = mean_length

    # Increment the progress bar
    progress.increment(f'Testing gradient table alterations ({index+1} of {len(variants)})')

  progress.done()

  # Sort the list to find the best gradient configuration(s)
  sorted_variants = list(variants)
  sorted_variants.sort(reverse=True, key=sort_key)
  best_variant = sorted_variants[0]

  if best_variant.is_default():
    meanlength_ratio = 1.0
    app.console('Absence of manipulation of the gradient table resulted in the maximal mean length')
  else:
    meanlength_ratio = meanlength_default / best_variant.mean_length
    app.console(f'Optimal variant: "{best_variant}"')
    app.console(f'Ratio of mean length of empirical data to mean length of best candidate: {meanlength_ratio:.3f}')

  # Provide a printout of the mean streamline length of each gradient table manipulation
  if app.ARGS.all:
    def pad_transpose(string):
      return string.ljust(7, ' ').rjust(9, ' ')
    if app.ARGS.notranspose:
      sys.stdout.write('Mean length     Shuffle\n')
      for variant in sorted_variants:
        sys.stdout.write(f'  {variant.mean_length:5.2f}      {variant.str_shuffle_pretty()}\n')
    elif app.ARGS.shuffle == 'none':
      sys.stdout.write('Mean length   Transform\n')
      for variant in sorted_variants:
        sys.stdout.write(f'  {variant.mean_length:5.2f}       {pad_transpose(variant.str_transpose_preshuffle())}\n')
    else:
      sys.stdout.write('Mean length   Transform        Shuffle       Transform\n')
      for variant in sorted_variants:
        sys.stdout.write(f'  {variant.mean_length:5.2f}'
                         f'       {pad_transpose(variant.str_transpose_preshuffle())}'
                         f'     {variant.str_shuffle_pretty()}'
                         f'    {pad_transpose(variant.str_transpose_postshuffle())}\n')
    sys.stdout.flush()

  # Write comprehensive results to a file
  if app.ARGS.out:
    if app.ARGS.out.suffix.lower() == '.tsv':
      delimiter = '\t'
      quote = ''
    else:
      delimiter = ','
      quote = '"'
    with open(app.ARGS.out, 'w', encoding='utf-8') as outfile:
      outfile.write(f'Pre-shuffle transpose'
                    f'{delimiter}Shuffling basis'
                    f'{delimiter}Axis shuffle'
                    f'{delimiter}Post-shuffle transpose'
                    f'{delimiter}Mean length\n')
      for variant in variants:
        outfile.write(f'{variant.str_transpose_preshuffle()}'
                      f'{delimiter}{variant.shuffle_basis}'
                      f'{delimiter}{quote}{variant.str_shuffle()}{quote}'
                      f'{delimiter}{variant.str_transpose_postshuffle()}'
                      f'{delimiter}{variant.mean_length}\n')

  # If requested, extract what has been detected as the best gradient table, and
  #   export it in the format requested by the user
  grad_export_option = app.dwgrad_export_options()
  if grad_export_option:
    grad_import_option = None
    if best_variant.is_default():
      grad_import_option = ['-grad', 'grad.b']
    elif best_variant.out_format() == 'bvec':
      grad_import_option = ['-fslgrad', f'bvecs_{best_variant}', 'bvals']
    else:
      assert best_variant.out_format() == 'real'
      grad_import_option = ['-grad', f'grad_{best_variant}.b']
    run.command(['mrinfo', 'data.mif'] + grad_import_option + grad_export_option,
                force=app.FORCE_OVERWRITE)

  if meanlength_ratio < app.ARGS.threshold:
    raise MRtrixError('Streamline tractography indicates possibly incorrect gradient table')
