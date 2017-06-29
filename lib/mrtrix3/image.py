# A collection of functions for extracting information from images. Mostly these involve
#   calling the relevant MRtrix3 binaries in order to parse image headers / process image
#   data, rather than trying to duplicate support for all possible image formats natively
#   in Python.

def check3DNonunity(image_path):
  from mrtrix3 import app
  dim = header(image_path).size
  if len(dim) < 3:
    app.error('Image \'' + image_path + '\' does not contain 3 spatial dimensions')
  if min(dim) == 1:
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
    self.__dict__.update(elements)
    if not self.keyval:
      self.keyval = { }

def header(image_path):
  result = _Header(image_path)
  return result



# Check to see whether the fundamental header properties of two images match
# Inputs can be either _Header class instances, or file paths
def match(image_one, image_two):
  import math
  from mrtrix3 import app
  if not isinstance(image_one, _Header):
    if not type(image_one) is str:
      app.error('Error trying to test \'' + str(image_one) + '\': Not an image header or file path')
    image_one = header(image_one)
  if not isinstance(image_two, _Header):
    if not type(image_two) is str:
      app.error('Error trying to test \'' + str(image_two) + '\': Not an image header or file path')
    image_two = header(image_two)
  debug_prefix = '\'' + image_one.name + '\' \'' + image_two.name + '\''
  # Image dimensions
  if not image_one.size == image_two.size:
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



def statistic(image_path, statistic, mask_path = ''):
  import subprocess
  from mrtrix3 import app, run
  command = [ run.exeName(run.versionMatch('mrstats')), image_path, '-output', statistic ]
  if mask_path:
    command.extend([ '-mask', mask_path ])
  if app._verbosity > 1:
    app.console('Command: \'' + ' '.join(command) + '\' (piping data to local storage)')
  proc = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=None)
  result, err = proc.communicate()
  result = result.rstrip().decode('utf-8')
  if app._verbosity > 1:
    app.console('Result: ' + result)
  return result

