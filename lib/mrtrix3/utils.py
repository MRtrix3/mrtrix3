# Utility functions that don't logically fit into any other module



# Return a boolean flag to indicate whether or not script is being run on a Windows machine
def is_windows(): #pylint: disable=unused-variable
  import platform
  system = platform.system().lower()
  return any(system.startswith(s) for s in [ 'mingw', 'msys', 'nt', 'windows' ])



# Load key-value entries from the comments within a text file
def load_keyval(filename, **kwargs): #pylint: disable=unused-variable
  import re

  comments = kwargs.pop('comments', '#')
  encoding = kwargs.pop('encoding', 'latin1')
  errors = kwargs.pop('errors', 'ignore')
  if kwargs:
    raise TypeError('Unsupported keyword arguments passed to utils.load_keyval(): ' + str(kwargs))

  def decode(line):
    if isinstance(line, bytes):
      line = line.decode(encoding, errors=errors)
    return line

  if comments:
    regex_comments = re.compile('|'.join(comments))

  res = {}
  with open(filename, 'rb') as infile:
    for line in infile.readlines():
      line = decode(line)
      if comments:
        line = regex_comments.split(line, maxsplit=1)[0]
        if len(line) < 2:
          continue
        name, var = line.rstrip().partition(":")[::2]
        if name in res.keys():
          res[name].append(var.split())
        else:
          res[name] = var.split()
  return res
