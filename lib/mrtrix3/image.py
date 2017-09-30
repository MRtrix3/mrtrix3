# A collection of functions for extracting information from images. Mostly these involve
#   calling the relevant MRtrix3 binaries in order to parse image headers / process image
#   data, rather than trying to duplicate support for all possible image formats natively
#   in Python.

def check3DNonunity(image_path):
  from mrtrix3 import app
  dim = [ int(i) for i in headerField(image_path, 'size').strip().split() ]
  if len(dim) < 3:
    app.error('Image \'' + image_path + '\' does not contain 3 spatial dimensions')
  if min(dim) == 1:
    app.error('Image \'' + image_path + '\' does not contain 3D spatial information (axis with size 1)')



def headerField(image_path, field):
  import subprocess
  from mrtrix3 import app, run
  command = [ run.exeName(run.versionMatch('mrinfo')), image_path, '-' + field ]
  if app._verbosity > 1:
    app.console('Command: \'' + ' '.join(command) + '\' (piping data to local storage)')
  proc = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=None)
  result, err = proc.communicate()
  result = result.rstrip().decode('utf-8')
  if app._verbosity > 1:
    if '\n' in result:
      app.console('Result: (' + str(result.count('\n')+1) + ' lines)')
      app.debug(result)
    else:
      app.console('Result: ' + result)
  return result



def headerKeyValue(image_path, key):
  import subprocess
  from mrtrix3 import app, run
  command = [ run.exeName(run.versionMatch('mrinfo')), image_path, '-property', key ]
  if app._verbosity > 1:
    app.console('Command: \'' + ' '.join(command) + '\' (piping data to local storage)')
  proc = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=None)
  result, err = proc.communicate()
  result = result.rstrip().decode('utf-8')
  if app._verbosity > 1:
    app.console('Result: ' + result)
  return result



def match(image_one, image_two, max_dim=0):
  import math
  from mrtrix3 import app
  debug_prefix = '\'' + image_one + '\' \'' + image_two + '\''
  # Image dimensions
  one_size = [ int(i) for i in headerField(image_one, 'size').split() ]
  two_size = [ int(i) for i in headerField(image_two, 'size').split() ]
  if max_dim:
    if max_dim > min(len(one_size), len(two_size)):
      app.debug(debug_prefix + ' dimensionality less than specified maximum (' + str(max_dim) + ')')
      return False
  else:
    if not len(one_size) == len(two_size):
      app.debug(debug_prefix + ' dimensionality mismatch (' + str(len(one_size)) + ' vs. ' + str(len(two_size)) + ')')
      return False
    max_dim = len(one_size)
  if not one_size[:max_dim] == two_size[:max_dim]:
    app.debug(debug_prefix + ' axis size mismatch (' + str(one_size) + ' ' + str(two_size) + ')')
    return False
  # Voxel size
  one_spacing = [ float(f) for f in headerField(image_one, 'vox').split() ]
  two_spacing = [ float(f) for f in headerField(image_two, 'vox').split() ]
  for one, two in zip(one_spacing[:max_dim], two_spacing[:max_dim]):
    if one and two and not math.isnan(one) and not math.isnan(two):
      if (abs(two-one) / (0.5*(one+two))) > 1e-04:
        app.debug(debug_prefix + ' voxel size mismatch (' + str(one_spacing) + ' ' + str(two_spacing) + ')')
        return False
  # Image transform
  one_transform = [ float(f) for f in headerField(image_one, 'transform').replace('\n', ' ').replace(',', ' ').split() ]
  two_transform = [ float(f) for f in headerField(image_two, 'transform').replace('\n', ' ').replace(',', ' ').split() ]
  for one, two in zip(one_transform, two_transform):
    if abs(one-two) > 1e-4:
      app.debug(debug_prefix + ' transform mismatch (' + str(one_transform) + ' ' + str(two_transform) + ')')
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

