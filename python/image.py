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

# A collection of functions for extracting information from images. Mostly these involve
#   calling the relevant MRtrix3 binaries in order to parse image headers / process image
#   data, rather than trying to duplicate support for all possible image formats natively
#   in Python.


import json, math, os, subprocess
from collections import namedtuple
from mrtrix3 import MRtrixError



# Class for importing header information from an image file for reading
class Header:
  def __init__(self, image_path):
    from mrtrix3 import app, path, run #pylint: disable=import-outside-toplevel
    filename = path.name_temporary('json')
    command = [ run.exe_name(run.version_match('mrinfo')), image_path, '-json_all', filename ]
    if app.VERBOSITY > 1:
      app.console('Loading header for image file \'' + image_path + '\'')
    app.debug(str(command))
    result = subprocess.call(command, stdout=None, stderr=None)
    if result:
      raise MRtrixError('Could not access header information for image \'' + image_path + '\'')
    try:
      with open(filename, 'r', encoding='utf-8') as json_file:
        data = json.load(json_file)
    except UnicodeDecodeError:
      with open(filename, 'r', encoding='utf-8') as json_file:
        data = json.loads(json_file.read().decode('utf-8', errors='replace'))
    os.remove(filename)
    try:
      #self.__dict__.update(data)
      # Load the individual header elements manually, for a couple of reasons:
      # - So that pylint knows that they'll be there
      # - Write to private members, and give read-only access
      self._name = data['name']
      self._size = data['size']
      self._spacing = data['spacing']
      self._strides = data['strides']
      self._format = data['format']
      self._datatype = data['datatype']
      self._intensity_offset = data['intensity_offset']
      self._intensity_scale = data['intensity_scale']
      self._transform = data['transform']
      if not 'keyval' in data or not data['keyval']:
        self._keyval = { }
      else:
        self._keyval = data['keyval']
    except Exception as exception:
      raise MRtrixError('Error in reading header information from file \'' + image_path + '\'') from exception
    app.debug(str(vars(self)))

  def name(self):
    return self._name
  def size(self):
    return self._size
  def spacing(self):
    return self._spacing
  def strides(self):
    return self._strides
  def format(self):
    return self._format
  def datatype(self):
    return self._datatype
  def intensity_offset(self):
    return self._intensity_offset
  def intensity_scale(self):
    return self._intensity_scale
  def transform(self):
    return self._transform
  def keyval(self):
    return self._keyval

  def is_sh(self):
    """ is 4D image, with more than one volume, matching the number of coefficients of SH series """
    from mrtrix3 import sh  #pylint: disable=import-outside-toplevel
    if len(self._size) != 4 or self._size[3] == 1:
      return False
    return sh.n_for_l(sh.l_for_n(self._size[3])) == self._size[3]



# From a string corresponding to a NIfTI axis & direction code,
#   yield a 3-vector corresponding to that axis and direction
# Note that unlike phaseEncoding.direction(), this does not accept
#   an axis index, nor a phase-encoding indication string (e.g. AP);
#   it only accepts NIfTI codes, i.e. i, i-, j, j-, k, k-
def axis2dir(string): #pylint: disable=unused-variable
  from mrtrix3 import app #pylint: disable=import-outside-toplevel
  if string == 'i':
    direction = [1,0,0]
  elif string == 'i-':
    direction = [-1,0,0]
  elif string == 'j':
    direction = [0,1,0]
  elif string == 'j-':
    direction = [0,-1,0]
  elif string == 'k':
    direction = [0,0,1]
  elif string == 'k-':
    direction = [0,0,-1]
  else:
    raise MRtrixError('Unrecognized NIfTI axis & direction specifier: ' + string)
  app.debug(string + ' -> ' + str(direction))
  return direction



# Determine whether or not an image contains at least three axes, the first three of which
#   have dimension greater than one: This means that the data can plausibly represent
#   spatial information, and 3D interpolation can be performed
def check_3d_nonunity(image_in): #pylint: disable=unused-variable
  from mrtrix3 import app #pylint: disable=import-outside-toplevel
  if not isinstance(image_in, Header):
    if not isinstance(image_in, str):
      raise MRtrixError('Error trying to test \'' + str(image_in) + '\': Not an image header or file path')
    image_in = Header(image_in)
  if len(image_in.size()) < 3:
    raise MRtrixError('Image \'' + image_in.name() + '\' does not contain 3 spatial dimensions')
  if min(image_in.size()[:3]) == 1:
    raise MRtrixError('Image \'' + image_in.name() + '\' does not contain 3D spatial information (has axis with size 1)')
  app.debug('Image \'' + image_in.name() + '\' is >= 3D, and does not contain a unity spatial dimension')



# Despite being able to import the entire image header contents using the header()
# function, there are still some functionalities in mrinfo that can prove useful.
# Therefore, provide this function to execute mrinfo and get just the information of
#   interest. Note however that parsing the output of mrinfo e.g. into list / numerical
#   form is not performed by this function.
def mrinfo(image_path, field): #pylint: disable=unused-variable
  from mrtrix3 import app, run #pylint: disable=import-outside-toplevel
  command = [ run.exe_name(run.version_match('mrinfo')), image_path, '-' + field ]
  if app.VERBOSITY > 1:
    app.console('Command: \'' + ' '.join(command) + '\' (piping data to local storage)')
  with subprocess.Popen(command, stdout=subprocess.PIPE, stderr=None) as proc:
    result, dummy_err = proc.communicate()
  result = result.rstrip().decode('utf-8')
  if app.VERBOSITY > 1:
    app.console('Result: ' + result)
  # Don't exit on error; let the calling function determine whether or not
  #   the absence of the key is an issue
  return result



# Check to see whether the fundamental header properties of two images match
# Inputs can be either _Header class instances, or file paths
def match(image_one, image_two, **kwargs): #pylint: disable=unused-variable, too-many-return-statements
  from mrtrix3 import app #pylint: disable=import-outside-toplevel
  up_to_dim = kwargs.pop('up_to_dim', 0)
  check_transform = kwargs.pop('check_transform', True)
  if kwargs:
    raise TypeError('Unsupported keyword arguments passed to image.match(): ' + str(kwargs))
  if not isinstance(image_one, Header):
    if not isinstance(image_one, str):
      raise MRtrixError('Error trying to test \'' + str(image_one) + '\': Not an image header or file path')
    image_one = Header(image_one)
  if not isinstance(image_two, Header):
    if not isinstance(image_two, str):
      raise MRtrixError('Error trying to test \'' + str(image_two) + '\': Not an image header or file path')
    image_two = Header(image_two)
  debug_prefix = '\'' + image_one.name() + '\' \'' + image_two.name() + '\''
  # Handle possibility of only checking up to a certain axis
  if up_to_dim:
    if up_to_dim > min(len(image_one.size()), len(image_two.size())):
      app.debug(debug_prefix + ' dimensionality less than specified maximum (' + str(up_to_dim) + ')')
      return False
  else:
    if len(image_one.size()) != len(image_two.size()):
      app.debug(debug_prefix + ' dimensionality mismatch (' + str(len(image_one.size())) + ' vs. ' + str(len(image_two.size())) + ')')
      return False
    up_to_dim = len(image_one.size())
  # Image dimensions
  if not image_one.size()[:up_to_dim] == image_two.size()[:up_to_dim]:
    app.debug(debug_prefix + ' axis size mismatch (' + str(image_one.size()) + ' ' + str(image_two.size()) + ')')
    return False
  # Voxel size
  for one, two in zip(image_one.spacing()[:up_to_dim], image_two.spacing()[:up_to_dim]):
    if one and two and not math.isnan(one) and not math.isnan(two):
      if (abs(two-one) / (0.5*(one+two))) > 1e-04:
        app.debug(debug_prefix + ' voxel size mismatch (' + str(image_one.spacing()) + ' ' + str(image_two.spacing()) + ')')
        return False
  # Image transform
  if check_transform:
    for line_one, line_two in zip(image_one.transform(), image_two.transform()):
      for one, two in zip(line_one[:3], line_two[:3]):
        if abs(one-two) > 1e-4:
          app.debug(debug_prefix + ' transform (rotation) mismatch (' + str(image_one.transform()) + ' ' + str(image_two.transform()) + ')')
          return False
      if abs(line_one[3]-line_two[3]) > 1e-2:
        app.debug(debug_prefix + ' transform (translation) mismatch (' + str(image_one.transform()) + ' ' + str(image_two.transform()) + ')')
        return False
  # Everything matches!
  app.debug(debug_prefix + ' image match')
  return True



# Computes image statistics using mrstats.
# Return will be a list of ImageStatistics instances if there is more than one volume
#   and allvolumes=True is not set; a single ImageStatistics instance otherwise
ImageStatistics = namedtuple('ImageStatistics', 'mean median std std_rv min max count')
IMAGE_STATISTICS = [ 'mean', 'median', 'std', 'std_rv', 'min', 'max', 'count' ]

def statistics(image_path, **kwargs): #pylint: disable=unused-variable
  from mrtrix3 import app, run #pylint: disable=import-outside-toplevel
  mask = kwargs.pop('mask', None)
  allvolumes = kwargs.pop('allvolumes', False)
  ignorezero = kwargs.pop('ignorezero', False)
  if kwargs:
    raise TypeError('Unsupported keyword arguments passed to image.statistics(): ' + str(kwargs))

  command = [ run.exe_name(run.version_match('mrstats')), image_path ]
  for stat in IMAGE_STATISTICS:
    command.extend([ '-output', stat ])
  if mask:
    command.extend([ '-mask', mask ])
  if allvolumes:
    command.append('-allvolumes')
  if ignorezero:
    command.append('-ignorezero')
  if app.VERBOSITY > 1:
    app.console('Command: \'' + ' '.join(command) + '\' (piping data to local storage)')

  try:
    from subprocess import DEVNULL #pylint: disable=import-outside-toplevel
  except ImportError:
    DEVNULL = open(os.devnull, 'wb') #pylint: disable=consider-using-with
  with subprocess.Popen(command, stdout=subprocess.PIPE, stderr=DEVNULL) as proc:
    stdout = proc.communicate()[0]
    try:
      DEVNULL.close()
    except AttributeError:
      pass
    if proc.returncode:
      raise MRtrixError('Error trying to calculate statistics from image \'' + image_path + '\'')

  stdout_lines = [ line.strip() for line in stdout.decode('cp437').splitlines() ]
  result = [ ]
  for line in stdout_lines:
    line = line.replace('N/A', 'nan').split()
    assert len(line) == len(IMAGE_STATISTICS)
    result.append(ImageStatistics(float(line[0]), float(line[1]), float(line[2]), float(line[3]), float(line[4]), float(line[5]), int(line[6])))
  if len(result) == 1:
    result = result[0]
  if app.VERBOSITY > 1:
    app.console('Result: ' + str(result))
  return result
