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

# Various utility functions related to vector and matrix data



import io, itertools, os, re
from mrtrix3 import COMMAND_HISTORY_STRING, MRtrixError


_TRANSFORM_LAST_ROW = [ 0.0, 0.0, 0.0, 1.0 ]



# Dot product between two vectors / matrices
def dot(input_a, input_b): #pylint: disable=unused-variable
  if not input_a:
    if input_b:
      raise MRtrixError('Dimension mismatch (0 vs. ' + str(len(input_b)) + ')')
    return [ ]
  if is_2d_matrix(input_a):
    if not is_2d_matrix(input_b):
      raise MRtrixError('Both inputs must be either 1D vectors or 2D matrices')
    if len(input_a[0]) != len(input_b):
      raise MRtrixError('Invalid dimensions for matrix dot product(' + \
                            str(len(input_a)) + 'x' + str(len(input_a[0])) + ' vs. ' + \
                            str(len(input_b)) + 'x' + str(len(input_b[0])) + ')')
    return [[sum(x*y for x,y in zip(a_row,b_col)) for b_col in zip(*input_b)] for a_row in input_a]
  if is_2d_matrix(input_b):
    raise MRtrixError('Both inputs must be either 1D vectors or 2D matrices')
  if len(input_a) != len(input_b):
    raise MRtrixError('Dimension mismatch (' + str(len(input_a)) + ' vs. ' + str(len(input_b)) + ')')
  return sum(x*y for x,y in zip(input_a, input_b))



# Check whether a data entity is a valid 2D matrix
def is_2d_matrix(data):
  if not data:
    return False
  if not isinstance(data, list):
    return False
  if not isinstance(data[0], list):
    return False
  columns = len(data[0])
  for row in data[1:]:
    if len(row) != columns:
      return False
  return True



# Transpose a matrix / vector
def transpose(data): #pylint: disable=unused-variable
  if not is_2d_matrix(data):
    raise TypeError('matrix.transpose() operates on 2D matrices only')
  return [ list(x) for x in zip(*data) ]




################################################################
# Functions for loading and saving numeric data from / to file #
################################################################



# Load a text file containing numeric data
#   (can be a different number of entries in each row)
def load_numeric(filename, **kwargs):
  dtype = kwargs.pop('dtype', float)
  # By default support the same set of delimiters at load as the MRtrix3 C++ code
  delimiter = kwargs.pop('delimiter', ' ,;\t')
  comments = kwargs.pop('comments', '#')
  encoding = kwargs.pop('encoding', 'latin1')
  errors = kwargs.pop('errors', 'ignore')
  if kwargs:
    raise TypeError('Unsupported keyword arguments passed to matrix.load_numeric(): ' + str(kwargs))

  def decode(line):
    if isinstance(line, bytes):
      line = line.decode(encoding, errors=errors)
    return line

  if comments:
    regex_comments = re.compile('|'.join(comments))

  data = [ ]
  with open(filename, 'rb') as infile:
    for line in infile.readlines():
      line = decode(line)
      if comments:
        line = regex_comments.split(line, maxsplit=1)[0]
      line = line.strip()
      if line:
        if len(delimiter) == 1:
          data.append([dtype(a) for a in line.split(delimiter) if a])
        else:
          data.append([dtype(a) for a in [''.join(g) for k, g in itertools.groupby(line, lambda c : c in delimiter) if not k ]])

  if not data:
    return None

  return data



# Load a text file containing specifically matrix data
def load_matrix(filename, **kwargs): #pylint: disable=unused-variable
  data = load_numeric(filename, **kwargs)
  columns = len(data[0])
  for line in data[1:]:
    if len(line) != columns:
      raise MRtrixError('Inconsistent number of columns in matrix text file "' + filename + '"')
  return data



# Load a text file containing specifically affine spatial transformation data
def load_transform(filename, **kwargs): #pylint: disable=unused-variable
  data = load_matrix(filename, **kwargs)
  if len(data) == 4:
    if any(a!=b for a, b in zip(data[3], _TRANSFORM_LAST_ROW)):
      raise MRtrixError('File "' + filename + '" does not contain a valid transform (fourth line contains values other than "0,0,0,1")')
  elif len(data) == 3:
    data.append(_TRANSFORM_LAST_ROW)
  else:
    raise MRtrixError('File "' + filename + '" does not contain a valid transform (must contain 3 or 4 lines)')
  if len(data[0]) != 4:
    raise MRtrixError('File "' + filename + '" does not contain a valid transform (must contain 4 columns)')
  return data



# Load a text file containing specifically vector data
def load_vector(filename, **kwargs): #pylint: disable=unused-variable
  data = load_matrix(filename, **kwargs)
  if len(data) == 1:
    return data[0]
  for line in data:
    if len(line) != 1:
      raise MRtrixError('File "' + filename + '" does not contain vector data (multiple columns detected)')
  return [ line[0] for line in data ]



# Save numeric data to a text file
def save_numeric(filename, data, **kwargs):
  fmt = kwargs.pop('fmt', '%.15g')
  delimiter = kwargs.pop('delimiter', ' ')
  newline = kwargs.pop('newline', '\n')
  add_to_command_history = bool(kwargs.pop('add_to_command_history', True))
  header = kwargs.pop('header', { })
  footer = kwargs.pop('footer', { })
  comments = kwargs.pop('comments', '# ')
  encoding = kwargs.pop('encoding', None)
  force = kwargs.pop('force', False)
  if kwargs:
    raise TypeError('Unsupported keyword arguments passed to matrix.save_numeric(): ' + str(kwargs))

  if not force and os.path.exists(filename):
    raise MRtrixError('output file "' + filename + '" already exists (use -force option to force overwrite)')

  encode_args = {'errors': 'ignore'}
  if encoding:
    encode_args['encoding'] = encoding

  if header:
    if isinstance(header, str):
      header = { 'comments' : header }
    elif isinstance(header, list):
      header = { 'comments' : '\n'.join(str(entry) for entry in header) }
    elif isinstance(header, dict):
      header = dict((key, str(value)) for key, value in header.items())
    else:
      raise TypeError('Unrecognised input to matrix.save_numeric() using "header=" option')
  else:
    header = { }

  if add_to_command_history and COMMAND_HISTORY_STRING:
    if 'command_history' in header:
      header['command_history'] += '\n' + COMMAND_HISTORY_STRING
    else:
      header['command_history'] = COMMAND_HISTORY_STRING

  if footer:
    if isinstance(footer, str):
      footer = { 'comments' : footer }
    elif isinstance(footer, list):
      footer = { 'comments' : '\n'.join(str(entry) for entry in footer) }
    elif isinstance(footer, dict):
      footer = dict((key, str(value)) for key, value in footer.items())
    else:
      raise TypeError('Unrecognised input to matrix.save_numeric() using "footer=" option')
  else:
    footer = { }

  open_mode = os.O_WRONLY | os.O_CREAT
  if force:
    open_mode = open_mode | os.O_TRUNC
  else:
    open_mode = open_mode | os.O_EXCL
  file_descriptor = os.open(filename, open_mode)
  with io.open(file_descriptor, 'wb') as outfile:
    for key, value in sorted(header.items()):
      for line in value.splitlines():
        outfile.write((comments + key + ': ' + line + newline).encode(**encode_args))

    if data:
      if isinstance(data[0], list):
        # input is 2D
        for row in data:
          row_fmt = delimiter.join([fmt, ] * len(row))
          outfile.write(((row_fmt % tuple(row) + newline).encode(**encode_args)))

      else:
        # input is 1D
        fmt = delimiter.join([fmt, ] * len(data))
        outfile.write(((fmt % tuple(data) + newline).encode(**encode_args)))

    for key, value in sorted(footer.items()):
      for line in value.splitlines():
        outfile.write((comments + key + ': ' + line + newline).encode(**encode_args))



# Save matrix data to a text file
def save_matrix(filename, data, **kwargs): #pylint: disable=unused-variable
  if not isinstance(data, list) or not isinstance(data[0], list):
    raise TypeError('Input to matrix.save_matrix() must be a 2D matrix')
  columns = len(data[0])
  for line in data[1:]:
    if len(line) != columns:
      raise TypeError('Input to matrix.save_matrix() must be a 2D matrix')
  kwargs['fmt'] = kwargs['fmt'] if 'fmt' in kwargs else '%18.15f'
  save_numeric(filename, data, **kwargs)



# Save an affine transformation matrix to a text file
def save_transform(filename, data, **kwargs): #pylint: disable=unused-variable
  if not isinstance(data, list) or not isinstance(data[0], list):
    raise TypeError('Input to matrix.save_transform() must be a 3x4 or 4x4 matrix')
  for line in data:
    if len(line) != 4:
      raise TypeError('Input to matrix.save_transform() must be a 3x4 or 4x4 matrix')
  if len(data) == 4:
    if any(a!=b for a, b in zip(data[3], _TRANSFORM_LAST_ROW)):
      raise TypeError('Input to matrix.save_transform() is not a valid affine matrix (fourth line contains values other than "0,0,0,1")')
    save_matrix(filename, data, **kwargs)
  elif len(data) == 3:
    padded_data = data[:]
    padded_data.append(_TRANSFORM_LAST_ROW)
    save_matrix(filename, data, **kwargs)
  else:
    raise TypeError('Input to matrix.save_matrix() must be a 3x4 or 4x4 matrix')



# Save vector data to a text file
def save_vector(filename, data, **kwargs): #pylint: disable=unused-variable
  if not isinstance(data, list) or (data and isinstance(data[0], list)):
    raise TypeError('Input to matrix.save_vector() must be a list of numbers')
  # Prefer column vectors to row vectors in text files
  if 'delimiter' not in kwargs:
    kwargs['delimiter'] = '\n'
  save_numeric(filename, data, **kwargs)
