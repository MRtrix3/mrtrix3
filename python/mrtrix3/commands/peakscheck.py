#!/usr/bin/python3

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

import itertools
import os
import sys

from mrtrix3 import COMMAND_HISTORY_STRING, MRtrixError #pylint: disable=no-name-in-module
from mrtrix3 import app, image, run #pylint: disable=no-name-in-module



EUCLIDEAN_FLIPS = (None, 0, 1, 2)
EUCLIDEAN_PERMUTATIONS = ((0,1,2), (0,2,1), (1,0,2), (1,2,0), (2,0,1), (2,1,0))

THREEVECTOR_OPERATION_SETS = ([],
                              ['euclidean_shuffle'],
                              ['euclidean_transform'],
                              ['euclidean_shuffle', 'euclidean_transform'],
                              ['euclidean_transform', 'euclidean_shuffle'])

# - 'convert' here refers specifically to a spherical to cartesian conversion
SPHERICAL_OPERATION_SETS = (['convert'],
                            ['convert', 'euclidean_shuffle'],
                            ['convert', 'euclidean_transform'],
                            ['spherical_shuffle', 'convert'],
                            ['spherical_transform', 'convert'],
                            ['convert', 'euclidean_shuffle', 'euclidean_transform'],
                            ['convert', 'euclidean_transform', 'euclidean_shuffle'],
                            ['spherical_shuffle', 'convert', 'euclidean_transform'],
                            ['spherical_shuffle', 'spherical_transform', 'convert'],
                            ['spherical_transform', 'convert', 'euclidean_shuffle'],
                            ['spherical_transform', 'spherical_shuffle', 'convert'])

DEFAULT_NUMBER = 10000
DEFAULT_THRESHOLD = 0.95

FORMATS = ('spherical', 'unitspherical', '3vector', 'unit3vector')
DEFAULT_FORMAT = '3vector'



def usage(cmdline): #pylint: disable=unused-variable
  cmdline.set_author('Robert E. Smith (robert.smith@florey.edu.au)')
  cmdline.set_synopsis('Check the orientations of an image containing discrete fibre orientations')
  cmdline.add_description(
      'MRtrix3 expects "peaks" images to be stored'
      ' using the real / scanner space axes as reference.'
      ' There are three possible sources of error in this interpretation:'
      ' 1. There may be erroneous axis flips and/or permutations,'
      ' but within the real / scanner space reference.'
      ' 2. The image data may provide fibre orientations'
      ' with reference to the image axes rather than real / scanner space.'
      ' Here there are two additional possibilities:'
      ' 2a. There may be requisite axis permutations / flips'
      ' to be applied to the image data *before* transforming them to real / scanner space.'
      ' 2b. There may be requisite axis permutations / flips'
      ' to be applied to the image data *after* transforming them to real / scanner space.')
  cmdline.add_argument('input',
                       type=app.Parser.ImageIn(),
                       help='The input fibre orientations image to be checked')
  cmdline.add_argument('-mask',
                       type=app.Parser.ImageIn(),
                       help='Provide a mask image within which to seed & constrain tracking')
  cmdline.add_argument('-number',
                       type=app.Parser.Int(1),
                       default=DEFAULT_NUMBER,
                       help='Set the number of tracks to generate for each test')
  cmdline.add_argument('-threshold',
                       type=app.Parser.Float(0.0, 1.0),
                       default=DEFAULT_THRESHOLD,
                       help='Modulate thresold on the ratio of empirical to maximal mean length'
                            ' to issue an error')
  cmdline.add_argument('-in_format',
                       choices=FORMATS,
                       default=DEFAULT_FORMAT,
                       help='The format in which peak orientations are specified;'
                            f' one of: {",".join(FORMATS)}')
  cmdline.add_argument('-noshuffle',
                       action='store_true',
                       help='Do not evaluate possibility of requiring shuffles of axes or angles;'
                            ' only consider prospective transforms'
                            ' from alternative reference frames to real / scanner space')
  cmdline.add_argument('-notransform',
                       action='store_true',
                       help='Do not evaluate possibility of requiring transform of peak orientations'
                            ' from image to real / scanner space;'
                            ' only consider prospective shuffles of axes or angles')
  cmdline.flag_mutually_exclusive_options(['noshuffle', 'notransform'])
  cmdline.add_argument('-all',
                       action='store_true',
                       help='Print table containing all results to standard output')
  cmdline.add_argument('-out_table',
                       type=app.Parser.FileOut(),
                       help='Write text file with table containing all results')



class Operation:
  @staticmethod
  def prettytype():
    assert False
  def prettyparams(self):
    assert False
  #def cmd(self, nfixels, in_path, out_path):
  #  assert False

class EuclideanShuffle(Operation):
  @staticmethod
  def prettytype():
    return 'Euclidean axis shuffle'
  def __init__(self, flip, permutations):
    self.flip = flip
    self.permutations = permutations
  def __format__(self, _):
    if self.no_flip():
      assert not self.no_permutations()
      return f'perm{"".join(map(str, self.permutations))}'
    if self.no_permutations():
      return f'flip{self.flip}'
    return f'flip{self.flip}perm{"".join(map(str, self.permutations))}'
  def prettyparams(self):
    result = ''
    if self.flip is not None:
      result = f'Flip axis {self.flip}'
      if not self.no_permutations():
        result += '; '
    if not self.no_permutations():
      result += f'Permute axes: ({",".join(map(str, self.permutations))})'
    return result
  def no_flip(self):
    return self.flip is None
  def no_permutations(self):
    return self.permutations == (0,1,2)
  def cmd(self, nfixels, in_path, out_path):
    # Below is modified copy-paste from code prior to "new variants"
    volume_list = [ None ] * (3 * nfixels)
    for in_volume_index in range(0, 3*nfixels):
      fixel_index = in_volume_index // 3
      in_component = in_volume_index - 3*fixel_index
      # Do we need to invert this item prior to permutation?
      flip = self.flip == in_component
      flip_string = 'flip' if flip else ''
      # What should be the index of this image after permutation has taken place?
      out_component = self.permutations[in_component]
      # Where will this volume reside in the output image series?
      out_volume_index = 3*fixel_index + out_component
      assert volume_list[out_volume_index] is None
      # Create the image
      temppath = f'{os.path.splitext(in_path)[0]}_{flip_string}{in_volume_index}_{out_volume_index}.mif'
      cmd = ['mrconvert', in_path,
             '-coord', '3', f'{in_volume_index}',
             '-axes', '0,1,2',
             '-config', 'RealignTransform', 'false']
      if flip:
        cmd.extend(['-', '|', 'mrcalc', '-', '-1.0', '-mult',
                    '-config', 'RealignTransform', 'false'])
      cmd.append(temppath)
      run.command(cmd)
      volume_list[out_volume_index] = temppath
    assert all(item is not None for item in volume_list)
    run.command(['mrcat', volume_list, out_path,
                 '-axis', '3',
                 '-config', 'RealignTransform', 'false'])
    for item in volume_list:
      os.remove(item)

class EuclideanTransform(Operation):
  @staticmethod
  def prettytype():
    return 'Euclidean reference axis change'
  def __init__(self, reference):
    self.reference = reference
  def __format__(self, _):
    return f'euctrans{self.reference}2xyz'
  def prettyparams(self):
    return f'{self.reference} to xyz'
  def cmd(self, _, in_path, out_path):
    run.command(['peaksconvert', in_path, out_path,
                 '-in_reference', self.reference,
                 '-in_format', '3vector',
                 '-out_reference', 'xyz',
                 '-out_format', '3vector',
                 '-config', 'RealignTransform', 'false'])

class SphericalShuffle(Operation):
  @staticmethod
  def prettytype():
    return 'Spherical angle shuffle'
  def __init__(self, is_unit, swap, el2in):
    self.is_unit = is_unit
    self.swap = swap
    self.el2in = el2in
  def __format__(self, _):
    if self.swap:
      if self.el2in:
        return 'swapandel2in'
      return 'swap'
    return 'el2in'
  def prettyparams(self):
    result = ''
    if self.swap:
      result = 'Swap angles'
      if self.el2in:
        result += '; '
    if self.el2in:
      result += ' Transform elevation to inclination'
    return result
  def volsperfixel(self):
    return 2 if self.is_unit else 3
  def cmd(self, nfixels, in_path, out_path):
    # Below is modified copy-paste from code prior to "new variants"
    num_volumes = self.volsperfixel() * nfixels
    volume_list = [ None ] * num_volumes
    for in_volume_index in range(0, num_volumes):
      fixel_index = in_volume_index // self.volsperfixel()
      in_component = in_volume_index - (self.volsperfixel() * fixel_index)
      # Is this a spherical angle that needs to be permuted?
      if self.swap and not (not self.is_unit and in_component == 0):
        # If unit, 0->1 and 1->0
        # If not unit, 1->2 and 2->1
        out_component = (1 if self.is_unit else 3) - in_component
      else:
        out_component = in_component
      # Where will this volume reside in the output image series?
      out_volume_index = 3*fixel_index + out_component
      assert volume_list[out_volume_index] is None
      # Create the image
      temppath = f'{os.path.splitext(in_path)[0]}_{in_volume_index}{"el2in" if self.el2in else ""}_{out_volume_index}.mif'
      cmd = ['mrconvert', in_path,
             '-coord', '3', f'{in_volume_index}',
             '-axes', '0,1,2',
             '-config', 'RealignTransform', 'false']
      # Do we need to apply the elevation->inclination transform?
      if self.el2in and out_component == self.volsperfixel() - 1:
        cmd.extend(['-', '|', 'mrcalc', '0.5', 'pi', '-mult', '-', '-sub',
                    '-config', 'RealignTransform', 'false'])
      cmd.append(temppath)
      run.command(cmd)
      volume_list[out_volume_index] = temppath
    assert all(item is not None for item in volume_list)
    run.command(['mrcat', volume_list, out_path,
                 '-axis', '3',
                 '-config', 'RealignTransform', 'false'])
    for item in volume_list:
      os.remove(item)

class SphericalTransform(Operation):
  @staticmethod
  def prettytype():
    return 'Spherical reference axis change'
  def __init__(self, is_unit, reference):
    self.is_unit = is_unit
    self.reference = reference
  def __format__(self, _):
    return f'sphtrans{self.reference}2xyz'
  def prettyparams(self):
    return f'{self.reference} to xyz'
  def cmd(self, _, in_path, out_path):
    run.command(['peaksconvert', in_path, out_path,
                 '-in_reference', self.reference,
                 '-in_format', 'unitspherical' if self.is_unit else 'spherical',
                 '-out_reference', 'xyz',
                 '-out_format', 'unitspherical' if self.is_unit else 'spherical',
                 '-config', 'RealignTransform', 'false'])

class Spherical2Cartesian(Operation):
  @staticmethod
  def prettytype():
    return 'Spherical-to-cartesian transformation'
  def __init__(self, is_unit):
    self.is_unit = is_unit
  def __format__(self, _):
    return 'sph2cart'
  def prettyparams(self):
    return 'N/A'
  def cmd(self, _, in_path, out_path):
    run.command(['peaksconvert', in_path, out_path,
                 '-in_format', 'unitspherical' if self.is_unit else 'spherical',
                 '-out_format', 'unit3vector' if self.is_unit else '3vector',
                 '-config', 'RealignTransform', 'false'])



class Variant():
  def __init__(self, operations):
    assert isinstance(operations, list)
    assert all(isinstance(item, Operation) for item in operations)
    self.operations = operations
  def __format__(self, _):
    if not self.operations:
      return 'none'
    return '_'.join(f'{item}' for item in self.operations)
  def is_default(self):
    if not self.operations:
      return True
    return len(self.operations) == 1 and isinstance(self.operations[0], Spherical2Cartesian)

def sort_key(item):
  assert item.mean_length is not None
  return item.mean_length



def execute(): #pylint: disable=unused-variable

  image_dimensions = image.Header(app.ARGS.input).size()
  if len(image_dimensions) != 4:
    raise MRtrixError('Input image must be a 4D image')
  if min(image_dimensions) == 1:
    raise MRtrixError('Cannot perform tractography on an image with a unity dimension')
  num_volumes = image_dimensions[3]
  if app.ARGS.in_format in ('unit3vector', '3vector'):
    num_fixels = num_volumes // 3
    if 3 * num_fixels != num_volumes:
      raise MRtrixError(f'Number of input volumes ({num_volumes})'
                        ' not a valid peaks image:'
                        ' must be a multiple of 3')
  elif app.ARGS.in_format == 'spherical':
    num_fixels = num_volumes // 3
    if 3 * num_fixels != num_volumes:
      raise MRtrixError(f'Number of input volumes ({num_volumes})'
                        ' not a valid spherical coordinates image:'
                        ' must be a multiple of 3')
  elif app.ARGS.in_format == 'unitspherical':
    num_fixels = num_volumes // 2
    if 2 * num_fixels != num_volumes:
      raise MRtrixError(f'Number of input volumes ({num_volumes})'
                        ' not a valid unit spherical coordinates image:'
                        ' must be a multiple of 2')
  else:
    assert False

  is_unit = app.ARGS.in_format in ('unitspherical', 'unit3vector')

  app.activate_scratch_dir()

  # Unlike dwigradcheck, here we're going to be performing manual permutation & flipping of volumes
  # Therefore, we'd actually prefer to *not* have contiguous memory across volumes
  # Also, in order for subsequent reference transforms to be valid,
  #   we need for the strides to not be modified by MRtrix3 at the load stage
  run.command(f'mrconvert {app.ARGS.input} data.mif'
              ' -datatype float32'
              ' -config RealignTransform false')

  if app.ARGS.mask:
    run.command(f'mrconvert {app.ARGS.mask} mask.mif'
                ' -datatype bit'
                ' -config RealignTransform false')

  # Generate a brain mask if we weren't provided with one
  if not app.ARGS.mask:
    run.command('mrcalc data.mif -abs - -config RealignTransform false | '
                'mrmath - max -axis 3 - -config RealignTransform false | '
                'mrthreshold - mask.mif -abs 0.0 -comparison gt -config RealignTransform false')

  # How many tracks are we going to generate?
  number_option = ['-select', str(app.ARGS.number)]

  operation_sets = THREEVECTOR_OPERATION_SETS \
                   if app.ARGS.in_format in ('unit3vector', '3vector') \
                   else SPHERICAL_OPERATION_SETS

  # To facilitate looping in order to generate all possible variants,
  #   pre-prepare lists of all possible configurations of each operation
  all_euclidean_shuffles = []
  for flip in EUCLIDEAN_FLIPS:
    for permutation in EUCLIDEAN_PERMUTATIONS:
      if flip is None and permutation == (0,1,2):
        continue
      all_euclidean_shuffles.append(EuclideanShuffle(flip, permutation))
  all_euclidean_transforms = [EuclideanTransform('ijk'),
                              EuclideanTransform('fsl')]
  all_spherical_shuffles = [SphericalShuffle(is_unit, True, False),
                            SphericalShuffle(is_unit, False, True),
                            SphericalShuffle(is_unit, True, True)]
  all_spherical_transforms = [SphericalTransform(is_unit, 'ijk'),
                              SphericalTransform(is_unit, 'fsl')]
  all_spherical2cartesian = [Spherical2Cartesian(is_unit),]

  variants = []
  for operation_set in operation_sets:
    if app.ARGS.noshuffle and any('shuffle' in item for item in operation_set):
      continue
    if app.ARGS.notransform and any('transform' in item for item in operation_set):
      continue
    operations_list = []
    for operation in operation_set:
      if operation == 'convert':
        operations_list.append(all_spherical2cartesian)
      elif operation == 'euclidean_shuffle':
        operations_list.append(all_euclidean_shuffles)
      elif operation == 'euclidean_transform':
        operations_list.append(all_euclidean_transforms)
      elif operation == 'spherical_shuffle':
        operations_list.append(all_spherical_shuffles)
      elif operation == 'spherical_transform':
        operations_list.append(all_spherical_transforms)
      else:
        assert False
    for variant in itertools.product(*operations_list):
      variants.append(Variant(list(variant)))
  app.debug(f'Complete list of variants for {app.ARGS.in_format} input format:')
  for variant in variants:
    app.debug(f'{variant}')

  progress = app.ProgressBar('Testing peaks orientation alterations'
                             + (f' (0 of {len(variants)})'
                                if sys.stderr.isatty()
                                else f' ({len(variants)} variants)'),
                             len(variants))
  meanlength_default = None
  for variant_index, variant in enumerate(variants):

    # For each variant,
    #   we now have to construct and execute the sequential set of commands necessary
    tmppath = None
    imagepath = 'data.mif'
    for operation_index, operation in enumerate(variant.operations):
      in_path = 'data.mif' if operation_index == 0 else tmppath
      imagepath = f'{os.path.splitext(in_path)[0]}_{operation}.mif'
      # Ensure that an intermediate output of one variant doesn't clash
      #   with the final output of another variant
      if operation_index == len(variant.operations) - 1:
        imagepath = imagepath[len('data_'):]
      operation.cmd(num_fixels, in_path, imagepath)
      if tmppath:
        run.function(os.remove, tmppath)
      tmppath = imagepath

    # Run the tracking experiment
    track_file_path = f'tracks_{variant}.tck'
    run.command(['tckgen', imagepath,
                 '-algorithm', 'fact',
                 '-seed_image', 'mask.mif',
                 '-mask', 'mask.mif',
                 '-minlength', '0',
                 '-downsample', '5',
                 '-config', 'RealignTransform', 'false',
                 track_file_path]
                + number_option)
    if imagepath != 'data.mif':
      app.cleanup(imagepath)

    # Get the mean track length & add to the database
    mean_length = float(run.command(['tckstats', track_file_path, '-output', 'mean', '-ignorezero']).stdout)
    variant.mean_length = mean_length
    app.cleanup(track_file_path)

    # Save result if this is the unmodified empirical gradient table
    if variant.is_default():
      assert meanlength_default is None
      meanlength_default = mean_length

    # Increment the progress bar
    progress.increment('Testing peaks orientation alterations'
                       f' ({variant_index+1} of {len(variants)})')

  progress.done()

  # Sort the list to find the best gradient configuration(s)
  sorted_variants = list(variants)
  sorted_variants.sort(reverse=True, key=sort_key)
  meanlength_max = sorted_variants[0].mean_length

  if sorted_variants[0].is_default():
    meanlength_ratio = 1.0
    app.console('Absence of manipulation of peaks orientations'
                ' resulted in the maximal mean length')
  else:
    meanlength_ratio = meanlength_default / meanlength_max
    app.console('Ratio of mean length of empirical data to mean length of best candidate:'
                f' {meanlength_ratio:.3f}')

  # Provide a printout of the mean streamline length of each orientation manipulation
  if app.ARGS.all:
    sys.stdout.write('Mean length     Variant\n')
    for variant in sorted_variants:
      sys.stdout.write(f'  {variant.mean_length:5.2f}      {variant}\n')
    sys.stdout.flush()

  # Write comprehensive results to a file
  if app.ARGS.out_table:
    if app.ARGS.out_table.suffix.lower() == '.tsv':
      delimiter = '\t'
      quote = ''
    else:
      delimiter = ','
      quote = '"'
    with open(app.ARGS.out_table, 'w', encoding='utf-8') as outfile:
      outfile.write(f'# command_history: {COMMAND_HISTORY_STRING}\n')
      outfile.write(f'{quote}Mean length{quote}{delimiter}'
                    f'{quote}Operation 1 type{quote}{delimiter}{quote}Operation 1 parameters{quote}{delimiter}'
                    f'{quote}Operation 2 type{quote}{delimiter}{quote}Operation 2 parameters{quote}{delimiter}'
                    f'{quote}Operation 3 type{quote}{delimiter}{quote}Operation 3 parameters{quote}{delimiter}\n')
      for variant in variants:
        outfile.write(f'{variant.mean_length}')
        for operation in variant.operations:
          outfile.write(f'{delimiter}{quote}{operation.prettytype()}{quote}{delimiter}{quote}{operation.prettyparams()}{quote}')
        outfile.write('\n')

  if meanlength_ratio < app.ARGS.threshold:
    raise MRtrixError('Streamline tractography indicates possibly incorrect interpretation of peak orientations')
