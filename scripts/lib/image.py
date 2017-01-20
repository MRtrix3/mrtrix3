# A collection of functions for extracting information from images. Mostly these involve
#   calling the relevant MRtrix3 binaries in order to parse image headers / process image
#   data, rather than trying to duplicate support for all possible image formats natively
#   in Python.

def headerField(image_path, field):
  import subprocess
  import lib.app
  import lib.message
  import lib.mrtrix
  command = [ lib.mrtrix.exeVersionMatch('mrinfo'), image_path, '-' + field ]
  if lib.app.verbosity > 1:
    lib.message.print('Command: \'' + ' '.join(command) + '\' (piping data to local storage)')
  proc = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=None)
  result, err = proc.communicate()
  result = result.rstrip().decode('utf-8')
  if lib.app.verbosity > 1:
    if '\n' in result:
      lib.message.print('Result: (' + str(result.count('\n')+1) + ' lines)')
      lib.message.debug(result)
    else:
      lib.message.print('Result: ' + result)
  return result



def headerKeyValue(image_path, key):
  import subprocess
  import lib.app
  import lib.message
  import lib.mrtrix
  command = [ lib.mrtrix.exeVersionMatch('mrinfo'), image_path, '-property', key ]
  if lib.app.verbosity > 1:
    lib.message.print('Command: \'' + ' '.join(command) + '\' (piping data to local storage)')
  proc = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=None)
  result, err = proc.communicate()
  result = result.rstrip().decode('utf-8')
  if lib.app.verbosity > 1:
    lib.message.print('Result: ' + result)
  return result



def match(image_one, image_two):
  import math
  import lib.message
  debug_prefix = '\'' + image_one + '\' \'' + image_two + '\''
  # Image dimensions
  one_dim = [ int(i) for i in headerField(image_one, 'size').split() ]
  two_dim = [ int(i) for i in headerField(image_two, 'size').split() ]
  if not one_dim == two_dim:
    lib.message.debug(debug_prefix + ' dimension mismatch (' + str(one_dim) + ' ' + str(two_dim) + ')')
    return False
  # Voxel size
  one_spacing = [ float(f) for f in headerField(image_one, 'vox').split() ]
  two_spacing = [ float(f) for f in headerField(image_two, 'vox').split() ]
  for one, two in zip(one_spacing, two_spacing):
    if one and two and not math.isnan(one) and not math.isnan(two):
      if (abs(two-one) / (0.5*(one+two))) > 1e-04:
        lib.message.debug(debug_prefix + ' voxel size mismatch (' + str(one_spacing) + ' ' + str(two_spacing) + ')')
        return False
  # Image transform
  one_transform = [ float(f) for f in headerField(image_one, 'transform').replace('\n', ' ').replace(',', ' ').split() ]
  two_transform = [ float(f) for f in headerField(image_two, 'transform').replace('\n', ' ').replace(',', ' ').split() ]
  for one, two in zip(one_transform, two_transform):
    if abs(one-two) > 1e-4:
      lib.message.debug(debug_prefix + ' transform mismatch (' + str(one_transform) + ' ' + str(two_transform) + ')')
      return False
  # Everything matches!
  debugMessage(debug_prefix + ' image match')
  return True



def statistic(image_path, statistic, mask_path = ''):
  import subprocess
  import lib.app
  import lib.message
  import lib.mrtrix
  command = [ lib.mrtrix.exeVersionMatch('mrstats'), image_path, '-output', statistic ]
  if mask_path:
    command.extend([ '-mask', mask_path ])
  if lib.app.verbosity > 1:
    lib.message.print('Command: \'' + ' '.join(command) + '\' (piping data to local storage)')
  proc = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=None)
  result, err = proc.communicate()
  result = result.rstrip().decode('utf-8')
  if lib.app.verbosity > 1:
    lib.message.print('Result: ' + result)
  return result

