# Functions relating to handling phase encoding information

# From a user-specified string, determine the axis and direction of phase encoding
def dir(string):
  from mrtrix3 import app
  pe_dir = ''
  try:
    PE_axis = abs(int(string))
    if PE_axis > 2:
      app.error('When specified as a number, phase encode axis must be either 0, 1 or 2 (positive or negative)')
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
      app.error('Unrecognized phase encode direction specifier: ' + string)
  app.debug(string + ' -> ' + str(pe_dir))
  return pe_dir



# Extract a phase-encoding scheme from a pre-loaded image header,
#   or from a path to the image
def getScheme(input):
  from mrtrix3 import app, image
  if not isinstance(input, image._Header):
    if not input is str:
      app.error('Error trying to derive phase-encoding scheme from \'' + str(input) + '\': Not an image header or file path')
    input = image.header(input)
  if 'pe_scheme' in input.keyval:
    app.debug(input.keyval['pe_scheme'])
    return input.keyval['pe_scheme']
  if not 'PhaseEncodingDirection' in input.keyval:
    return None
  line = dir(input.keyval['PhaseEncodingDirection'])
  if 'TotalReadoutTime' in input.keyval:
    line.append(input.keyval['TotalReadoutTime'])
  num_volumes = 1 if len(input.size()) < 4 else input.size[3]
  app.debug(str(line) + ' x ' + num_volumes + ' rows')
  return line * num_volumes

