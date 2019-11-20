# Copyright (c) 2008-2019 the MRtrix3 contributors.
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

# Functions relating to handling phase encoding information



from mrtrix3 import MRtrixError



# From a user-specified string, determine the axis and direction of phase encoding
def direction(string): #pylint: disable=unused-variable
  from mrtrix3 import app #pylint: disable=import-outside-toplevel
  pe_dir = ''
  try:
    pe_axis = abs(int(string))
    if pe_axis > 2:
      raise MRtrixError('When specified as a number, phase encode axis must be either 0, 1 or 2 (positive or negative)')
    reverse = (string.contains('-')) # Allow -0
    pe_dir = [0,0,0]
    if reverse:
      pe_dir[pe_axis] = -1
    else:
      pe_dir[pe_axis] = 1
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
      raise MRtrixError('Unrecognized phase encode direction specifier: ' + string)
  app.debug(string + ' -> ' + str(pe_dir))
  return pe_dir



# Extract a phase-encoding scheme from a pre-loaded image header,
#   or from a path to the image
def get_scheme(arg): #pylint: disable=unused-variable
  from mrtrix3 import app, image #pylint: disable=import-outside-toplevel
  if not isinstance(arg, image.Header):
    if not isinstance(arg, str):
      raise MRtrixError('Error trying to derive phase-encoding scheme from \'' + str(arg) + '\': Not an image header or file path')
    arg = image.Header(arg)
  if 'pe_scheme' in arg.keyval():
    app.debug(str(arg.keyval()['pe_scheme']))
    return arg.keyval()['pe_scheme']
  if 'PhaseEncodingDirection' not in arg.keyval():
    return None
  line = direction(arg.keyval()['PhaseEncodingDirection'])
  if 'TotalReadoutTime' in arg.keyval():
    line = [ float(value) for value in line ]
    line.append(float(arg.keyval()['TotalReadoutTime']))
  num_volumes = 1 if len(arg.size()) < 4 else arg.size()[3]
  app.debug(str(line) + ' x ' + str(num_volumes) + ' rows')
  return [ line ] * num_volumes
