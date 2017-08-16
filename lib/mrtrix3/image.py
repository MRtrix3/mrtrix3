# A collection of functions for extracting information from images. Mostly these involve
#   calling the relevant MRtrix3 binaries in order to parse image headers / process image
#   data, rather than trying to duplicate support for all possible image formats natively
#   in Python.

def check3DNonunity(image_in): #pylint: disable=unused-variable
  from mrtrix3 import app
  if not isHeader(image_in):
    if not isinstance(image_in, str):
      app.error('Error trying to test \'' + str(image_in) + '\': Not an image header or file path')
    image_in = header(image_in)
  if len(image_in.size) < 3:
    app.error('Image \'' + image_in.name + '\' does not contain 3 spatial dimensions')
  if min(image_in.size[:3]) == 1:
    app.error('Image \'' + image_in.name + '\' does not contain 3D spatial information (has axis with size 1)')
  app.debug('Image \'' + image_in.name + '\' is >= 3D, and does not contain a unity spatial dimension')



# Function to grab all contents of an image header
# Uses mrinfo's new -json_all option in order to grab all header information
#   from any image format supported by MRtrix3's C++ libraries
def header(image_path):
  import json, os, subprocess
  from mrtrix3 import app, path, run
  filename = path.newTemporary('json')
  command = [ run.exeName(run.versionMatch('mrinfo')), image_path, '-json_all', filename ]
  if app.verbosity > 1:
    app.console('Loading header for image file \'' + image_path + '\'')
  app.debug(str(command))
  result = subprocess.call(command, stdout=None, stderr=None)
  if result:
    app.error('Could not access header information for image \'' + image_path + '\'')
  with open(filename, 'r') as f:
    elements = json.load(f)

  class _Header:
    def __init__(self, data):
      #self.__dict__.update(data)
      # Load the individual header elements manually, so that pylint knows that they'll be there
      self.name = data['name']
      self.size = data['size']
      self.spacing = data['spacing']
      self.stride = data['stride']
      self.format = data['format']
      self.datatype = data['datatype']
      self.intensity_offset = data['intensity_offset']
      self.intensity_scale = data['intensity_scale']
      self.transform = data['transform']
      if not 'keyval' in data or not data['keyval']:
        self.keyval = { }

  try:
    result = _Header(elements)
  except:
    app.error('Error in reading header information from file \'' + image_path + '\'')
  os.remove(filename)
  app.debug(str(vars(result)))
  return result



# Function to test whether or not a Python object contains those flags expected
#   within an image header
def isHeader(obj):
  from mrtrix3 import app
  try:
    #pylint: disable=pointless-statement,too-many-boolean-expressions
    obj.name and obj.size and obj.spacing and obj.stride and obj.format and obj.datatype \
             and obj.intensity_offset and obj.intensity_scale and obj.transform and obj.keyval
    app.debug('\'' + str(obj) + '\' IS a header object')
    return True
  except:
    app.debug('\'' + str(obj) + '\' is NOT a header object')
    return False



# Despite being able to import the entire image header contents using the header()
# function, there are still some functionalities in mrinfo that can prove useful.
# Therefore, provide this function to execute mrinfo and get just the information of
#   interest. Note however that parsing the output of mrinfo e.g. into list / numerical
#   form is not performed by this function.
def mrinfo(image_path, field): #pylint: disable=unused-variable
  import subprocess
  from mrtrix3 import app, run
  command = [ run.exeName(run.versionMatch('mrinfo')), image_path, '-' + field ]
  if app.verbosity > 1:
    app.console('Command: \'' + ' '.join(command) + '\' (piping data to local storage)')
  proc = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=None)
  result, dummy_err = proc.communicate()
  result = result.rstrip().decode('utf-8')
  if app.verbosity > 1:
    app.console('Result: ' + result)
  return result



# Check to see whether the fundamental header properties of two images match
# Inputs can be either _Header class instances, or file paths
def match(image_one, image_two): #pylint: disable=unused-variable
  import math
  from mrtrix3 import app
  if not isHeader(image_one):
    if not isinstance(image_one, str):
      app.error('Error trying to test \'' + str(image_one) + '\': Not an image header or file path')
    image_one = header(image_one)
  if not isHeader(image_two):
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



# Computes image statistics using mrstats.
def statistic(image_path, stat, options=''): #pylint: disable=unused-variable
  import shlex, subprocess
  from mrtrix3 import app, run
  command = [ run.exeName(run.versionMatch('mrstats')), image_path, '-output', stat ]
  if options:
    command.extend(shlex.split(options))
  if app.verbosity > 1:
    app.console('Command: \'' + ' '.join(command) + '\' (piping data to local storage)')
  proc = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=None)
  result, dummy_err = proc.communicate()
  result = result.rstrip().decode('utf-8')
  if app.verbosity > 1:
    app.console('Result: ' + result)
  return result
