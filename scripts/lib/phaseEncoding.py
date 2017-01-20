# Functions relating to handling phase encoding information

# From a user-specified string, determine the axis and direction of phase encoding
def dir(string):
  import lib.message
  pe_dir = ''
  try:
    PE_axis = abs(int(string))
    if PE_axis > 2:
      lib.message.error('When specified as a number, phase encode axis must be either 0, 1 or 2 (positive or negative)')
    reverse = (string.contains('-')) # Allow -0
    pe_dir = [0,0,0]
    if reverse:
      pe_dir[PE_axis] = -1
    else:
      pe_dir[PE_axis] = 1
  except:
    string = string.lower()
    if string == 'lr':
      pe_dir = [1,0,0]
    elif string == 'rl':
      pe_dir = [-1,0,0]
    elif string == 'pa':
      pe_dir = [0,1,0]
    elif string == 'ap':
      pe_dir = [0,-1,0]
    elif string == 'is':
      pe_dir = [0,0,1]
    elif string == 'si':
      pe_dir = [0,0,-1]
    elif string == 'i':
      pe_dir = [1,0,0]
    elif string == 'i-':
      pe_dir = [-1,0,0]
    elif string == 'j':
      pe_dir = [0,1,0]
    elif string == 'j-':
      pe_dir = [0,-1,0]
    elif string == 'k':
      pe_dir = [0,0,1]
    elif string == 'k-':
      pe_dir = [0,0,-1]
    else:
      lib.message.error('Unrecognized phase encode direction specifier: ' + string)
  lib.message.debug(string + ' -> ' + str(pe_dir))
  return pe_dir



# Extract a phase-encoding acheme from an image header
def getScheme(image_path):
  import subprocess
  import lib.app
  import lib.message
  import lib.mrtrix
  command = [ lib.mrtrix.exeVersionMatch('mrinfo'), image_path, '-petable' ]
  if lib.app.verbosity > 1:
    lib.message.print('Command: \'' + ' '.join(command) + '\' (piping data to local storage)')
  proc = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=None)
  result, err = proc.communicate()
  result = result.rstrip().decode('utf-8')
  if result:
    result = [ [ float(f) for f in line.split() ] for line in result.split('\n') ]
  if lib.app.verbosity > 1:
    if not result:
      lib.message.print('Result: No phase encoding table found')
    else:
      lib.message.print('Result: ' + str(len(result)) + ' x ' + str(len(result[0])) + ' table')
      lib.message.debug(str(result))
  return result

