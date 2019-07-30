# Various utility functions / classes that don't sensibly slot into any other module



# A simple wrapper class for executing a set of commands or functions of some known length,
#   generating and managing a progress bar as it does so
# Can use in one of two ways:
# - Construct using a progress bar message, and the number of commands / functions that are to be executed;
#     each is then executed by calling member functions command() and function(), which
#     use the corresponding functions in the mrtrix3.run module
# - Construct using a progress bar message, and a list of command strings to run;
#     all commands within the list will be executed sequentially within the constructor
class RunList(object): #pylint: disable=unused-variable
  def __init__(self, message, value):
    from mrtrix3 import app, run
    if isinstance(value, int):
      self.progress = app.ProgressBar(message, value)
      self.target_count = value
      self.counter = 0
      self.valid = True
    elif isinstance(value, list):
      assert all(isinstance(entry, str) for entry in value)
      self.progress = app.ProgressBar(message, len(value))
      for entry in value:
        run.command(entry)
        self.progress.increment()
      self.progress.done()
      self.valid = False
    else:
      raise TypeError('Construction of RunList class expects either an '
                      'integer (number of commands/functions to run), or a '
                      'list of command strings to execute')
  def command(self, cmd):
    from mrtrix3 import run
    assert self.valid
    run.command(cmd)
    self._increment()
  def function(self, func, *args, **kwargs):
    from mrtrix3 import run
    assert self.valid
    run.function(func, *args, **kwargs)
    self._increment()
  def _increment(self):
    self.counter += 1
    if self.counter == self.target_count:
      self.progress.done()
      self.valid = False
    else:
      self.progress.increment()



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
