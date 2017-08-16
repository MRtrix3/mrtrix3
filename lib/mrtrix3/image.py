# A collection of functions for extracting information from images. Mostly these involve
#   calling the relevant MRtrix3 binaries in order to parse image headers / process image
#   data, rather than trying to duplicate support for all possible image formats natively
#   in Python.

#pylint: disable=unused-variable
def check3DNonunity(image_path):
  from mrtrix3 import app
  dim = header(image_path).size
  if len(dim) < 3:
    app.error('Image \'' + image_path + '\' does not contain 3 spatial dimensions')
  if min(dim[:3]) == 1:
    app.error('Image \'' + image_path + '\' does not contain 3D spatial information (axis with size 1)')



# Function to grab all contents of an image header
# Uses mrinfo's new -json_all option in order to grab all header information
#   from any image format supported by MRtrix3's C++ libraries
class _Header:
  def __init__(self, image_path):
    import json, os, subprocess, tempfile
    from mrtrix3 import app, path, run
    filename = path.newTemporary('json')
    command = [ run.exeName(run.versionMatch('mrinfo')), image_path, '-json_all', filename ]
    if app._verbosity > 1:
      app.console('Loading header for image file \'' + image_path + '\'')
    app.debug(str(command))
    result = subprocess.call(command, stdout=None, stderr=None)
    if result:
      app.error('Could not access header information for image \'' + image_path + '\'')
    with open(filename, 'r') as f:
      elements = json.load(f)
    os.remove(filename)
    self.name = self.format = self.datatype = ''
    self.size = self.spacing = self.stride = []
    self.intensity_offset = 0.0
    self.intensity_scale = 1.0
    self.transform = [[]]
    self.keyval = { }
    self.__dict__.update(elements)
    #pylint: disable-msg=too-many-boolean-expressions
    if not self.name or not self.size or not self.spacing or not self.stride or not \
           self.format or not self.datatype or not self.transform:
      app.error('Error in reading header information from file \'' + image_path + '\'')

#pylint: disable=unused-variable
def header(image_path):
  result = _Header(image_path)
  return result



# Despite being able to import the entire image header contents using the header()
# function, there are still some functionalities in mrinfo that can prove useful.
# Therefore, provide this function to execute mrinfo and get just the information of
#   interest. Note however that parsing the output of mrinfo e.g. into list / numerical
#   form is not performed by this function.
#pylint: disable=unused-variable
def mrinfo(image_path, field):
  import subprocess
  from mrtrix3 import app, run
  command = [ run.exeName(run.versionMatch('mrinfo')), image_path, '-' + field ]
  if app._verbosity > 1:
    app.console('Command: \'' + ' '.join(command) + '\' (piping data to local storage)')
  proc = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=None)
  result, dummy_err = proc.communicate()
  result = result.rstrip().decode('utf-8')
  if app._verbosity > 1:
    app.console('Result: ' + result)
  return result



# Check to see whether the fundamental header properties of two images match
# Inputs can be either _Header class instances, or file paths
#pylint: disable=unused-variable
def match(image_one, image_two):
  import math
  from mrtrix3 import app
  if not isinstance(image_one, _Header):
    if not isinstance(image_one, str):
      app.error('Error trying to test \'' + str(image_one) + '\': Not an image header or file path')
    image_one = header(image_one)
  if not isinstance(image_two, _Header):
    if not isinstance(image_two, str):
      app.error('Error trying to test \'' + str(image_two) + '\': Not an image header or file path')
    image_two = header(image_two)
  debug_prefix = '\'' + image_one.name + '\' \'' + image_two.name + '\''
  # Image dimensions
  if image_one.size != image_two.size:
    app.debug(debug_prefix + ' dimension mismatch (' + str(image_one.size) + ' ' + str(image_two.size) + ')')
    return False
  # Voxel size
  for one, two in zip(image_one.spacing, image_two.spacing):
    if one and two and not math.isnan(one) and not math.isnan(two):
      if (abs(two-one) / (0.5*(one+two))) > 1e-04:
        app.debug(debug_prefix + ' voxel size mismatch (' + str(image_one.spacing) + ' ' + str(image_two.spacing) + ')')
        return False
  # Image transform
  for line_one, line_two in zip(image_one.transform, image_two.transform):
    for one, two in zip(line_one, line_two):
      if abs(one-two) > 1e-4:
        app.debug(debug_prefix + ' transform mismatch (' + str(image_one.transform) + ' ' + str(image_two.transform) + ')')
        return False
  # Everything matches!
  app.debug(debug_prefix + ' image match')
  return True



# TODO Change mask_path to instead receive a string of additional command-line options
#   (that way, -allvolumes can be used)
#pylint: disable=unused-variable
def statistic(image_path, stat, mask_path = ''):
  import subprocess
  from mrtrix3 import app, run
  command = [ run.exeName(run.versionMatch('mrstats')), image_path, '-output', stat ]
  if mask_path:
    command.extend([ '-mask', mask_path ])
  if app._verbosity > 1:
    app.console('Command: \'' + ' '.join(command) + '\' (piping data to local storage)')
  proc = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=None)
  result, dummy_err = proc.communicate()
  result = result.rstrip().decode('utf-8')
  if app._verbosity > 1:
    app.console('Result: ' + result)
  return result
