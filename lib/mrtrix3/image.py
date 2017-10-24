# A collection of functions for extracting information from images. Mostly these involve
#   calling the relevant MRtrix3 binaries in order to parse image headers / process image
#   data, rather than trying to duplicate support for all possible image formats natively
#   in Python.

# Class for importing header information from an image file for reading
class Header(object):
  def __init__(self, image_path):
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
      data = json.load(f)
    os.remove(filename)
    try:
      #self.__dict__.update(data)
      # Load the individual header elements manually, for a couple of reasons:
      # - So that pylint knows that they'll be there
      # - Write to private members, and give read-only access
      self._name = data['name']
      self._size = data['size']
      self._spacing = data['spacing']
      self._stride = data['stride']
      self._format = data['format']
      self._datatype = data['datatype']
      self._intensity_offset = data['intensity_offset']
      self._intensity_scale = data['intensity_scale']
      self._transform = data['transform']
      if not 'keyval' in data or not data['keyval']:
        self._keyval = { }
      else:
        self._keyval = data['keyval']
    except:
      app.error('Error in reading header information from file \'' + image_path + '\'')
    app.debug(str(vars(self)))

  def name(self):
    return self._name
  def size(self):
    return self._size
  def spacing(self):
    return self._spacing
  def stride(self):
    return self._stride
  def format(self):
    return self._format
  def datatype(self):
    return self.datatype
  def intensity_offset(self):
    return self._intensity_offset
  def intensity_scale(self):
    return self._intensity_scale
  def transform(self):
    return self._transform
  def keyval(self):
    return self._keyval



# Determine whether or not an image contains at least three axes, the first three of which
#   have dimension greater than one: This means that the data can plausibly represent
#   spatial information, and 3D interpolation can be performed
def check3DNonunity(image_in): #pylint: disable=unused-variable
  from mrtrix3 import app
  if not isinstance(image_in, Header):
    if not isinstance(image_in, str):
      app.error('Error trying to test \'' + str(image_in) + '\': Not an image header or file path')
    image_in = Header(image_in)
  if len(image_in.size()) < 3:
    app.error('Image \'' + image_in.name() + '\' does not contain 3 spatial dimensions')
  if min(image_in.size()[:3]) == 1:
    app.error('Image \'' + image_in.name() + '\' does not contain 3D spatial information (has axis with size 1)')
  app.debug('Image \'' + image_in.name() + '\' is >= 3D, and does not contain a unity spatial dimension')



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
  # Don't exit on error; let the calling function determine whether or not
  #   the absence of the key is an issue
  return result



# Check to see whether the fundamental header properties of two images match
# Inputs can be either _Header class instances, or file paths
def match(image_one, image_two, max_dim=0): #pylint: disable=unused-variable, too-many-return-statements
  import math
  from mrtrix3 import app
  if not isinstance(image_one, Header):
    if not isinstance(image_one, str):
      app.error('Error trying to test \'' + str(image_one) + '\': Not an image header or file path')
    image_one = Header(image_one)
  if not isinstance(image_two, Header):
    if not isinstance(image_two, str):
      app.error('Error trying to test \'' + str(image_two) + '\': Not an image header or file path')
    image_two = Header(image_two)
  debug_prefix = '\'' + image_one.name() + '\' \'' + image_two.name() + '\''
  # Handle possibility of only checking up to a certain axis
  if max_dim:
    if max_dim > min(len(image_one.size()), len(image_two.size())):
      app.debug(debug_prefix + ' dimensionality less than specified maximum (' + str(max_dim) + ')')
      return False
  else:
    if len(image_one.size()) != len(image_two.size()):
      app.debug(debug_prefix + ' dimensionality mismatch (' + str(len(image_one.size())) + ' vs. ' + str(len(image_two.size())) + ')')
      return False
    max_dim = len(image_one.size())
  # Image dimensions
  if not image_one.size()[:max_dim] == image_two.size()[:max_dim]:
    app.debug(debug_prefix + ' axis size mismatch (' + str(image_one.size()) + ' ' + str(image_two.size()) + ')')
    return False
  # Voxel size
  for one, two in zip(image_one.spacing()[:max_dim], image_two.spacing()[:max_dim]):
    if one and two and not math.isnan(one) and not math.isnan(two):
      if (abs(two-one) / (0.5*(one+two))) > 1e-04:
        app.debug(debug_prefix + ' voxel size mismatch (' + str(image_one.spacing()) + ' ' + str(image_two.spacing()) + ')')
        return False
  # Image transform
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
  if proc.returncode:
    app.error('Error trying to calculate statistic \'' + statistic + '\' from image \'' + image_path + '\'')
  return result
