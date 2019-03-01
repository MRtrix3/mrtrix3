import argparse, os, sys
import mrtrix3

# These global constants can / should be accessed directly by scripts:
# - 'ARGS' will contain the user's command-line inputs upon parsing of the command-line
# - 'DO_CLEANUP' will indicate whether or not the scratch directory will be deleted on script completion,
#   and whether intermediary files will be deleted when function cleanup() is called on them
# - 'exeName' will be the basename of the executed script
# - 'FORCE_OVERWRITE' will be True if the user has requested for existing output files to be
#   re-written, and at least one output target already exists
# - 'NUM_THREADS' will be updated based on the user specifying -nthreads at the command-line,
#   or will remain as None if nothing is explicitly specified
# - 'SCRATCH_DIR' will contain the path to any scratch directory constructed for the executable script,
#   or will be an empty string if none is requested
# - 'VERBOSITY' controls how much information will be printed at the terminal:
#   # 0 = quiet; 1 = default; 2 = info; 3 = debug
# - 'WORKING_DIR' will simply contain the current working directory when the executable script is run
ARGS = None
DO_CLEANUP = True
CONTINUE_OPTION = False
EXEC_NAME = os.path.basename(sys.argv[0])
FORCE_OVERWRITE = False #pylint: disable=unused-variable
NUM_THREADS = None #pylint: disable=unused-variable
SCRATCH_DIR = ''
VERBOSITY = 0 if 'MRTRIX_QUIET' in mrtrix3.CONFIG else 1
WORKING_DIR = os.getcwd()



# - 'CMDLINE' needs to be updated with any compulsory arguments and optional command-line inputs
#   necessary for the executable script to be added via its usage() function
#   It will however be passed to the calling executable as a parameter in the usage() function,
#   and should not be modified outside of this module outside of such functions
CMDLINE = None



_DEFAULT_COPYRIGHT = '''Copyright (c) 2008-2019 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.

Covered Software is provided under this License on an \"as is\"
basis, without warranty of any kind, either expressed, implied, or
statutory, including, without limitation, warranties that the
Covered Software is free of defects, merchantable, fit for a
particular purpose or non-infringing.
See the Mozilla Public License v. 2.0 for more details.

For more details, see http://www.mrtrix.org/.'''



_SIGNALS = { 'SIGALRM': 'Timer expiration',
             'SIGBUS' : 'Bus error: Accessing invalid address (out of storage space?)',
             'SIGFPE' : 'Floating-point arithmetic exception',
             'SIGHUP' : 'Disconnection of terminal',
             'SIGILL' : 'Illegal instruction (corrupt binary command file?)',
             'SIGINT' : 'Program manually interrupted by terminal',
             'SIGPIPE': 'Nothing on receiving end of pipe',
             'SIGPWR' : 'Power failure restart',
             'SIGQUIT': 'Received terminal quit signal',
             'SIGSEGV': 'Segmentation fault: Invalid memory reference',
             'SIGSYS' : 'Bad system call',
             'SIGTERM': 'Terminated by kill command',
             'SIGXCPU': 'CPU time limit exceeded',
             'SIGXFSZ': 'File size limit exceeded' }
           # Can't be handled; see https://bugs.python.org/issue9524
           # 'CTRL_C_EVENT': 'Terminated by user Ctrl-C input',
           # 'CTRL_BREAK_EVENT': Terminated by user Ctrl-Break input'



def execute(): #pylint: disable=unused-variable
  import inspect, shutil, signal
  from mrtrix3 import ANSI, MRtrixError, run
  global ARGS, CMDLINE, CONTINUE_OPTION, DO_CLEANUP, EXEC_NAME, FORCE_OVERWRITE, NUM_THREADS, SCRATCH_DIR, VERBOSITY, WORKING_DIR

  # Set up signal handlers
  for sig in _SIGNALS:
    try:
      signal.signal(getattr(signal, sig), handler)
    except:
      pass

  module = inspect.getmodule(inspect.stack()[-1][0])
  CMDLINE = Parser()
  try:
    module.usage(CMDLINE)
  except AttributeError:
    CMDLINE = None
    raise

  ########################################################################################################################
  # Note that everything after this point will only be executed if the script is designed to operate against the library #
  ########################################################################################################################

  # Deal with special command-line uses
  if len(sys.argv) == 1:
    CMDLINE.print_help()
    sys.exit(0)
  elif sys.argv[-1] == '__print_full_usage__':
    CMDLINE.print_full_usage()
    sys.exit(0)
  elif sys.argv[-1] == '__print_synopsis__':
    sys.stdout.write(CMDLINE._synopsis) #pylint: disable=protected-access
    sys.exit(0)
  elif sys.argv[-1] == '__print_usage_markdown__':
    CMDLINE.print_usage_markdown()
    sys.exit(0)
  elif sys.argv[-1] == '__print_usage_rst__':
    CMDLINE.print_usage_rst()
    sys.exit(0)

  # Do the main command-line input parsing
  ARGS = CMDLINE.parse_args()

  # Check for usage of standard options;
  #   need to check for the presence of these keys first, since there's a chance that
  #   an external script may have erased the standard options
  if hasattr(ARGS, 'help') and ARGS.help:
    CMDLINE.print_help()
    sys.exit(0)
  # Can't activate -version here: argparse.parse_args() will fail first
  #if hasattr(ARGS, 'version') and ARGS.version:
  #  CMDLINE.print_version()
  #  sys.exit(0)
  if hasattr(ARGS, 'force') and ARGS.force:
    FORCE_OVERWRITE = True
  if hasattr(ARGS, 'nocleanup') and ARGS.nocleanup:
    DO_CLEANUP = False
  if hasattr(ARGS, 'nthreads') and ARGS.nthreads:
    NUM_THREADS = ARGS.nthreads #pylint: disable=unused-variable
  if hasattr(ARGS, 'quiet') and ARGS.quiet:
    VERBOSITY = 0
  elif hasattr(ARGS, 'info') and ARGS.info:
    VERBOSITY = 2
  elif hasattr(ARGS, 'debug') and ARGS.debug:
    VERBOSITY = 3

  if hasattr(ARGS, 'cont') and ARGS.cont:
    CONTINUE_OPTION = True
    SCRATCH_DIR = os.path.abspath(ARGS.cont[0])
    # Prevent error from re-appearing at end of terminal output if script continuation results in success
    #   and -nocleanup is used
    try:
      os.remove(os.path.join(SCRATCH_DIR, 'error.txt'))
    except OSError:
      pass
    run.shared.set_continue(ARGS.cont[1])

  run.shared.verbosity = VERBOSITY
  run.shared.set_num_threads(NUM_THREADS)

  CMDLINE.print_citation_warning()

  return_code = 0
  try:
    module.execute()
  except (run.MRtrixCmdError, run.MRtrixFnError) as exception:
    return_code = 1
    is_cmd = isinstance(exception, run.MRtrixCmdError)
    DO_CLEANUP = False
    if SCRATCH_DIR:
      with open(os.path.join(SCRATCH_DIR, 'error.txt'), 'w') as outfile:
        outfile.write((exception.command if is_cmd else exception.function) + '\n\n' + str(exception) + '\n')
    exception_frame = inspect.getinnerframes(sys.exc_info()[2])[-2]
    try:
      filename = exception_frame.filename
      lineno = exception_frame.lineno
    except: # Prior to Python 3.5
      filename = exception_frame[1]
      lineno = exception_frame[2]
    sys.stderr.write('\n')
    sys.stderr.write(EXEC_NAME + ': ' + ANSI.error + '[ERROR] ' + (exception.command if is_cmd else exception.function) + ANSI.clear + ' ' + ANSI.debug + '(' + os.path.basename(filename) + ':' + str(lineno) + ')' + ANSI.clear + '\n')
    if str(exception):
      sys.stderr.write(EXEC_NAME + ': ' + ANSI.error + '[ERROR] Information from failed ' + ('command' if is_cmd else 'function') + ':' + ANSI.clear + '\n')
      sys.stderr.write(EXEC_NAME + ':\n')
      for line in str(exception).splitlines():
        sys.stderr.write(' ' * (len(EXEC_NAME)+2) + line + '\n')
      sys.stderr.write(EXEC_NAME + ':\n')
    else:
      sys.stderr.write(EXEC_NAME + ': ' + ANSI.error + '[ERROR] Failed ' + ('command' if is_cmd else 'function') + ' did not provide any output information' + ANSI.clear + '\n')
    sys.stderr.write(EXEC_NAME + ': ' + ANSI.error + '[ERROR] For debugging, inspect contents of scratch directory: ' + SCRATCH_DIR + ANSI.clear + '\n')
    sys.stderr.flush()
  except MRtrixError as exception:
    sys.stderr.write('\n')
    sys.stderr.write(EXEC_NAME + ': ' + ANSI.error + '[ERROR] ' + str(exception) + ANSI.clear + '\n')
    sys.stderr.flush()
  except Exception as exception: # pylint: disable=broad-except
    sys.stderr.write('\n')
    sys.stderr.write(EXEC_NAME + ': ' + ANSI.error + '[ERROR] Unhandled Python exception:' + ANSI.clear + '\n')
    sys.stderr.write(EXEC_NAME + ': ' + ANSI.error + '[ERROR]' + ANSI.clear + '   ' + ANSI.console + type(exception).__name__ + ': ' + str(exception) + ANSI.clear + '\n')
    traceback = sys.exc_info()[2]
    sys.stderr.write(EXEC_NAME + ': ' + ANSI.error + '[ERROR] Traceback:' + ANSI.clear + '\n')
    for item in inspect.getinnerframes(traceback)[1:]:
      try:
        filename = item.filename
        lineno = item.lineno
        function = item.function
        calling_code = item.code_context
      except AttributeError: # Prior to Python 3.5
        filename = item[1]
        lineno = item[2]
        function = item[3]
        calling_code = item[4]
      sys.stderr.write(EXEC_NAME + ': ' + ANSI.error + '[ERROR]' + ANSI.clear + '   ' + ANSI.console + filename + ':' + str(lineno) + ' (in ' + function + '())' + ANSI.clear + '\n')
      for line in calling_code:
        sys.stderr.write(EXEC_NAME + ': ' + ANSI.error + '[ERROR]' + ANSI.clear + '     ' + ANSI.debug + line.strip() + ANSI.clear + '\n')
  finally:
    run.shared.kill()
    if os.getcwd() != WORKING_DIR:
      if not return_code:
        console('Changing back to original directory (' + WORKING_DIR + ')')
      os.chdir(WORKING_DIR)
    if SCRATCH_DIR:
      if DO_CLEANUP:
        if not return_code:
          console('Deleting scratch directory (' + SCRATCH_DIR + ')')
        try:
          shutil.rmtree(SCRATCH_DIR)
        except OSError:
          pass
        SCRATCH_DIR = ''
      else:
        console('Scratch directory retained; location: ' + SCRATCH_DIR)
  sys.exit(return_code)



def check_output_path(path): #pylint: disable=unused-variable
  from mrtrix3 import MRtrixError
  global ARGS, FORCE_OVERWRITE, WORKING_DIR
  if not path:
    return
  abspath = os.path.abspath(os.path.join(WORKING_DIR, path))
  if os.path.exists(abspath):
    output_type = ''
    if os.path.isfile(abspath):
      output_type = ' file'
    elif os.path.isdir(abspath):
      output_type = ' directory'
    if FORCE_OVERWRITE:
      warn('Output' + output_type + ' \'' + path + '\' already exists; will be overwritten at script completion')
    else:
      raise MRtrixError('Output' + output_type + ' \'' + path + '\' already exists (use -force to override)')



def make_scratch_dir(): #pylint: disable=unused-variable
  import random, string
  from mrtrix3 import CONFIG, run
  global ARGS, CONTINUE_OPTION, EXEC_NAME, SCRATCH_DIR, WORKING_DIR
  if CONTINUE_OPTION:
    debug('Skipping scratch directory creation due to use of -continue option')
    return
  if SCRATCH_DIR:
    raise Exception('Cannot use multiple scratch directories')
  if hasattr(ARGS, 'scratch') and ARGS.scratch:
    dir_path = os.path.abspath(ARGS.scratch)
  else:
    # Defaulting to working directory since too many users have encountered storage issues
    dir_path = CONFIG.get('ScriptScratchDir', WORKING_DIR)
  prefix = CONFIG.get('ScriptScratchPrefix', EXEC_NAME + '-tmp-')
  SCRATCH_DIR = dir_path
  while os.path.isdir(SCRATCH_DIR):
    random_string = ''.join(random.choice(string.ascii_uppercase + string.digits) for x in range(6))
    SCRATCH_DIR = os.path.join(dir_path, prefix + random_string) + os.sep
  os.makedirs(SCRATCH_DIR)
  console('Generated scratch directory: ' + SCRATCH_DIR)
  with open(os.path.join(SCRATCH_DIR, 'cwd.txt'), 'w') as outfile:
    outfile.write(WORKING_DIR + '\n')
  with open(os.path.join(SCRATCH_DIR, 'command.txt'), 'w') as outfile:
    outfile.write(' '.join(sys.argv) + '\n')
  open(os.path.join(SCRATCH_DIR, 'log.txt'), 'w').close()
  # Also use this scratch directory for any piped images within run.command() calls,
  #   and for keeping a log of executed commands / functions
  run.shared.set_scratch_dir(SCRATCH_DIR)



def goto_scratch_dir(): #pylint: disable=unused-variable
  global SCRATCH_DIR
  if not SCRATCH_DIR:
    raise Exception('No scratch directory location set')
  if VERBOSITY:
    console('Changing to scratch directory (' + SCRATCH_DIR + ')')
  os.chdir(SCRATCH_DIR)



# This function can (and should in some instances) be called upon any file / directory
#   that is no longer required by the script. If the script has been instructed to retain
#   all intermediates, the resource will be retained; if not, it will be deleted (in particular
#   to dynamically free up storage space used by the script).
def cleanup(path): #pylint: disable=unused-variable
  import shutil
  global DO_CLEANUP, VERBOSITY
  if not DO_CLEANUP:
    return
  if isinstance(path, list):
    if len(path) == 1:
      cleanup(path[0])
      return
    if VERBOSITY > 2:
      console('Cleaning up ' + str(len(path)) + ' intermediate items: ' + str(path))
    for entry in path:
      if os.path.isfile(entry):
        func = os.remove
      elif os.path.isdir(entry):
        func = shutil.rmtree
      else:
        continue
      try:
        func(entry)
      except OSError:
        pass
    return
  if os.path.isfile(path):
    temporary_type = 'file'
    func = os.remove
  elif os.path.isdir(path):
    temporary_type = 'directory'
    func = shutil.rmtree
  else:
    debug('Unknown target \'' + path + '\'')
    return
  if VERBOSITY > 2:
    console('Cleaning up intermediate ' + temporary_type + ': \'' + path + '\'')
  try:
    func(path)
  except OSError:
    debug('Unable to cleanup intermediate ' + temporary_type + ': \'' + path + '\'')





# This function should be used to insert text into any mrconvert call writing an output image
#   to the user's requested destination
# It will ensure that the header contents of any output images reflect the execution of the script itself,
#   rather than its internal processes
def mrconvert_output_option(input_image): #pylint: disable=unused-variable
  from ._version import __version__
  global FORCE_OVERWRITE
  text = ' -copy_properties ' + input_image + ' -append_property command_history "' + sys.argv[0]
  for arg in sys.argv[1:]:
    text += ' \\"' + arg + '\\"'
  text += '  (version=' + __version__ + ')"'
  if FORCE_OVERWRITE:
    text += ' -force'
  return text






# A set of functions and variables for printing various information at the command-line.
def console(text): #pylint: disable=unused-variable
  from mrtrix3 import ANSI
  global VERBOSITY
  if VERBOSITY:
    sys.stderr.write(EXEC_NAME + ': ' + ANSI.console + text + ANSI.clear + '\n')

def debug(text): #pylint: disable=unused-variable
  import inspect
  from mrtrix3 import ANSI
  global EXEC_NAME, VERBOSITY
  if VERBOSITY <= 2:
    return
  outer_frames = inspect.getouterframes(inspect.currentframe())
  nearest = outer_frames[1]
  try:
    if len(outer_frames) == 2: # debug() called directly from script being executed
      try:
        origin = '(' + os.path.basename(nearest.filename) + ':' + str(nearest.lineno) + ')'
      except AttributeError: # Prior to Python 3.5
        origin = '(' + os.path.basename(nearest[1]) + ':' + str(nearest[2]) + ')'
    else: # Some function has called debug(): Get location of both that function, and where that function was invoked
      try:
        filename = nearest.filename
        funcname = nearest.function + '()'
      except AttributeError: # Prior to Python 3.5
        filename = nearest[1]
        funcname = nearest[3] + '()'
      modulename = inspect.getmodulename(filename)
      if modulename:
        funcname = modulename + '.' + funcname
      origin = funcname
      caller = outer_frames[2]
      try:
        origin += ' (from ' + os.path.basename(caller.filename) + ':' + str(caller.lineno) + ')'
      except AttributeError:
        origin += ' (from ' + os.path.basename(caller[1]) + ':' + str(caller[2]) + ')'
      finally:
        del caller
    sys.stderr.write(EXEC_NAME + ': ' + ANSI.debug + '[DEBUG] ' + origin + ': ' + text + ANSI.clear + '\n')
  finally:
    del nearest

def trace(): #pylint: disable=unused-variable
  import inspect
  from mrtrix3 import ANSI
  global VERBOSITY
  if VERBOSITY <= 2:
    return
  calling_frame = inspect.getouterframes(inspect.currentframe())[1]
  try:
    try:
      filename = calling_frame.filename
      lineno = calling_frame.lineno
    except AttributeError: # Prior to Python 3.5
      filename = calling_frame[1]
      lineno = calling_frame[2]
    sys.stderr.write(EXEC_NAME + ': ' + ANSI.debug + '[DEBUG] At ' + os.path.basename(filename) + ':' + str(lineno) + ANSI.clear + '\n')
  finally:
    del calling_frame

def var(*variables): #pylint: disable=unused-variable
  import inspect
  from mrtrix3 import ANSI
  global VERBOSITY
  if VERBOSITY <= 2:
    return
  calling_frame = inspect.getouterframes(inspect.currentframe())[1]
  try:
    try:
      calling_code = calling_frame.code_context[0]
      filename = calling_frame.filename
      lineno = calling_frame.lineno
    except AttributeError: # Prior to Python 3.5
      calling_code = calling_frame[4][0]
      filename = calling_frame[1]
      lineno = calling_frame[2]
    var_string = calling_code[calling_code.find('var(')+4:].rstrip('\n').rstrip(' ')[:-1].replace(',', ' ')
    var_names, var_values = var_string.split(), variables
    for name, value in zip(var_names, var_values):
      sys.stderr.write(EXEC_NAME + ': ' + ANSI.debug + '[DEBUG] (from ' + os.path.basename(filename) + ':' + str(lineno) + ') \'' + name + '\' = ' + str(value) + ANSI.clear + '\n')
  finally:
    del calling_frame

def warn(text): #pylint: disable=unused-variable
  from mrtrix3 import ANSI
  global EXEC_NAME
  sys.stderr.write(EXEC_NAME + ': ' + ANSI.warn + '[WARNING] ' + text + ANSI.clear + '\n')



# A class that can be used to display a progress bar on the terminal,
#   mimicing the behaviour of MRtrix3 binary commands
class ProgressBar(object): #pylint: disable=unused-variable

  BUSY = [ '.   ',
           ' .  ',
           '  . ',
           '   .',
           '  . ',
           ' .  ' ]

  def __init__(self, msg, target=0):
    from mrtrix3 import ANSI, run
    global EXEC_NAME, VERBOSITY
    self.counter = 0
    self.isatty = sys.stderr.isatty()
    self.iscomplete = False
    self.message = msg
    self.multiplier = 100.0/target if target else 0
    self.newline = '\n' if VERBOSITY > 1 else '' # If any more than default verbosity, may still get details printed in between progress updates
    self.old_value = 0
    self.orig_verbosity = VERBOSITY
    self.value = 0
    VERBOSITY = run.shared.verbosity = VERBOSITY - 1 if VERBOSITY else 0
    if self.isatty:
      sys.stderr.write(EXEC_NAME + ': ' + ANSI.execute + '[' + ('{0:>3}%'.format(self.value) if self.multiplier else ProgressBar.BUSY[0]) + ']' + ANSI.clear + ' ' + ANSI.console + self.message + '... ' + ANSI.clear + ANSI.lineclear + self.newline)
    else:
      sys.stderr.write(EXEC_NAME + ': ' + self.message + '... [' + self.newline)
    sys.stderr.flush()

  def increment(self, msg=''):
    import math
    assert not self.iscomplete
    self.counter += 1
    force_update = False
    if msg:
      self.message = msg
      force_update = True
    if self.multiplier:
      new_value = int(round(self.counter * self.multiplier))
    else:
      new_value = int(round(math.log(self.counter, 2))) + 1
    if new_value != self.value:
      self.old_value = self.value
      self.value = new_value
      force_update = True
    if force_update:
      self._update()

  def done(self):
    from mrtrix3 import ANSI, run
    global EXEC_NAME, VERBOSITY
    self.iscomplete = True
    if self.multiplier:
      self.value = 100
    if self.isatty:
      sys.stderr.write('\r' + EXEC_NAME + ': ' + ANSI.execute + '[' + ('100%' if self.multiplier else 'done') + ']' + ANSI.clear + ' ' + ANSI.console + self.message + ANSI.clear + ANSI.lineclear + '\n')
    else:
      if self.newline:
        sys.stderr.write(EXEC_NAME + ': ' + self.message + ' [' + ('=' * int(self.value/2)) + ']\n')
      else:
        sys.stderr.write('=' * (int(self.value/2) - int(self.old_value/2)) + ']\n')
    sys.stderr.flush()
    VERBOSITY = run.shared.verbosity = self.orig_verbosity

  def _update(self):
    from mrtrix3 import ANSI
    global EXEC_NAME
    assert not self.iscomplete
    if self.isatty:
      sys.stderr.write('\r' + EXEC_NAME + ': ' + ANSI.execute + '[' + ('{0:>3}%'.format(self.value) if self.multiplier else ProgressBar.BUSY[self.counter%6]) + ']' + ANSI.clear + ' ' + ANSI.console + self.message + '... ' + ANSI.clear + ANSI.lineclear + self.newline)
    else:
      if self.newline:
        sys.stderr.write(EXEC_NAME + ': ' + self.message + '... [' + ('=' * int(self.value/2)) + self.newline)
      else:
        sys.stderr.write('=' * (int(self.value/2) - int(self.old_value/2)))
    sys.stderr.flush()



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
    from mrtrix3 import run
    if isinstance(value, int):
      self.progress = ProgressBar(message, value)
      self.target_count = value
      self.counter = 0
      self.valid = True
    elif isinstance(value, list):
      assert all(isinstance(entry, str) for entry in value)
      self.progress = ProgressBar(message, len(value))
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



# The Parser class is responsible for setting up command-line parsing for the script.
#   This includes proper CONFIGuration of the argparse functionality, adding standard options
#   that are common for all scripts, providing a custom help page that is consistent with the
#   MRtrix3 binaries, and defining functions for exporting the help page for the purpose of
#   automated self-documentation.
class Parser(argparse.ArgumentParser):

  # pylint: disable=protected-access

  def __init__(self, *args_in, **kwargs_in):
    import inspect, subprocess
    global _DEFAULT_COPYRIGHT
    self._author = None
    self._citation_list = [ ]
    self._copyright = _DEFAULT_COPYRIGHT
    self._description = [ ]
    self._examples = [ ]
    self._external_citations = False
    self._mutually_exclusive_option_groups = [ ]
    self._synopsis = None
    kwargs_in['add_help'] = False
    argparse.ArgumentParser.__init__(self, *args_in, **kwargs_in)
    if 'parents' in kwargs_in:
      for parent in kwargs_in['parents']:
        self._citation_list.extend(parent._citation_list)
        self._external_citations = self._external_citations or parent._external_citations
    else:
      standard_options = self.add_argument_group('Standard options')
      standard_options.add_argument('-info', action='store_true', help='display information messages.')
      standard_options.add_argument('-quiet', action='store_true', help='do not display information messages or progress status. Alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.')
      standard_options.add_argument('-debug', action='store_true', help='display debugging messages.')
      self.flag_mutually_exclusive_options( [ 'info', 'quiet', 'debug' ] )
      standard_options.add_argument('-force', action='store_true', help='force overwrite of output files.')
      standard_options.add_argument('-nthreads', metavar='number', type=int, help='use this number of threads in multi-threaded applications (set to 0 to disable multi-threading)')
      standard_options.add_argument('-help', action='store_true', help='display this information page and exit.')
      standard_options.add_argument('-version', action='store_true', help='display version information and exit.')
      script_options = self.add_argument_group('Additional standard options for Python scripts')
      script_options.add_argument('-nocleanup', action='store_true', help='do not delete intermediate files during script execution, and do not delete scratch directory at script completion.')
      script_options.add_argument('-scratch', metavar='/path/to/scratch/', help='manually specify the path in which to generate the scratch directory.')
      script_options.add_argument('-continue', nargs=2, dest='cont', metavar=('<ScratchDir>', '<LastFile>'), help='continue the script from a previous execution; must provide the scratch directory path, and the name of the last successfully-generated file.')
    module_file = inspect.getsourcefile(inspect.stack()[-1][0])
    self._is_project = os.path.abspath(os.path.join(os.path.dirname(module_file), os.pardir, 'lib', 'mrtrix3', 'app.py')) != os.path.abspath(__file__)
    try:
      process = subprocess.Popen ([ 'git', 'describe', '--abbrev=8', '--dirty', '--always' ], cwd=os.path.abspath(os.path.join(os.path.dirname(module_file), os.pardir)), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
      self._git_version = process.communicate()[0]
      self._git_version = str(self._git_version.decode(errors='ignore')).strip() if process.returncode == 0 else 'unknown'
    except OSError:
      self._git_version = 'unknown'

  def set_author(self, text):
    self._author = text

  def set_synopsis(self, text):
    self._synopsis = text

  def add_citation(self, condition, reference, is_external): #pylint: disable=unused-variable
    self._citation_list.append( (condition, reference) )
    if is_external:
      self._external_citations = True

  def add_description(self, text): #pylint: disable=unused-variable
    self._description.append(text)

  def add_example_usage(self, title, code, description = ''): #pylint: disable=unused-variable
    self._examples.append( (title, code, description) )

  def set_copyright(self, text): #pylint: disable=unused-variable
    self._copyright = text

  # Mutually exclusive options need to be added before the command-line input is parsed
  def flag_mutually_exclusive_options(self, options, required=False): #pylint: disable=unused-variable
    if not isinstance(options, list) or not isinstance(options[0], str):
      raise Exception('Parser.flagMutuallyExclusiveOptions() only accepts a list of strings')
    self._mutually_exclusive_option_groups.append( (options, required) )

  def parse_args(self):
    if not self._author:
      raise Exception('Script author MUST be set in script\'s usage() function')
    if not self._synopsis:
      raise Exception('Script synopsis MUST be set in script\'s usage() function')
    if '-version' in sys.argv:
      self.print_version()
      sys.exit(0)
    result = argparse.ArgumentParser.parse_args(self)
    self._check_mutex_options(result)
    if self._subparsers:
      for alg in self._subparsers._group_actions[0].choices:
        self._subparsers._group_actions[0].choices[alg]._check_mutex_options(result)
    return result

  def print_citation_warning(self):
    # If a subparser has been invoked, the subparser's function should instead be called,
    #   since it might have had additional citations appended
    global ARGS
    if self._subparsers:
      subparser = getattr(ARGS, self._subparsers._group_actions[0].dest)
      for alg in self._subparsers._group_actions[0].choices:
        if alg == subparser:
          self._subparsers._group_actions[0].choices[alg].print_citation_warning()
          return
    if self._citation_list:
      console('')
      citation_warning = 'Note that this script makes use of commands / algorithms that have relevant articles for citation'
      if self._external_citations:
        citation_warning += '; INCLUDING FROM EXTERNAL SOFTWARE PACKAGES'
      citation_warning += '. Please consult the help page (-help option) for more information.'
      console(citation_warning)
      console('')

  # Overloads argparse.ArgumentParser function to give a better error message on failed parsing
  def error(self, text):
    import shlex
    for entry in sys.argv:
      if '-help'.startswith(entry):
        self.print_help()
        sys.exit(0)
    if self.prog and len(shlex.split(self.prog)) == len(sys.argv): # No arguments provided to subparser
      self.print_help()
      sys.exit(0)
    usage = self.format_usage()
    if self._subparsers:
      for alg in self._subparsers._group_actions[0].choices:
        if alg == sys.argv[1]:
          usage = self._subparsers._group_actions[0].choices[alg].format_usage()
          continue
    sys.stderr.write('\nError: %s\n' % text)
    sys.stderr.write('Usage: ' + usage + '\n')
    sys.stderr.write('       (Run ' + self.prog + ' -help for more information)\n\n')
    sys.stderr.flush()
    sys.exit(1)

  def _check_mutex_options(self, args_in):
    for group in self._mutually_exclusive_option_groups:
      count = 0
      for option in group[0]:
        # Checking its presence is not adequate; by default, argparse adds these members to the namespace
        # Need to test if more than one of these options DIFFERS FROM ITS DEFAULT
        # Will need to loop through actions to find it manually
        if hasattr(args_in, option):
          for arg in self._actions:
            if arg.dest == option:
              if not getattr(args_in, option) == arg.default:
                count += 1
              break
      if count > 1:
        sys.stderr.write('\nError: You cannot use more than one of the following options: ' + ', '.join([ '-' + o for o in group[0] ]) + '\n')
        sys.stderr.write('(Consult the help page for more information: ' + self.prog + ' -help)\n\n')
        sys.stderr.flush()
        sys.exit(1)
      if group[1] and not count:
        sys.stderr.write('\nError: One of the following options must be provided: ' + ', '.join([ '-' + o for o in group[0] ]) + '\n')
        sys.stderr.write('(Consult the help page for more information: ' + self.prog + ' -help)\n\n')
        sys.stderr.flush()
        sys.exit(1)

  def format_usage(self):
    argument_list = [ ]
    trailing_ellipsis = ''
    if self._subparsers:
      argument_list.append(self._subparsers._group_actions[0].dest)
      trailing_ellipsis = ' ...'
    for arg in self._positionals._group_actions:
      if arg.metavar:
        argument_list.append(arg.metavar)
      else:
        argument_list.append(arg.dest)
    return self.prog + ' ' + ' '.join(argument_list) + ' [ options ]' + trailing_ellipsis

  def print_help(self):
    import subprocess, textwrap
    from mrtrix3 import CONFIG
    from ._version import __version__

    def bold(text):
      return ''.join( c + chr(0x08) + c for c in text)

    def underline(text):
      return ''.join( '_' + chr(0x08) + c for c in text)

    wrapper_args = textwrap.TextWrapper(width=80, initial_indent='', subsequent_indent='                     ')
    wrapper_other = textwrap.TextWrapper(width=80, initial_indent='     ', subsequent_indent='     ')
    if self._is_project:
      text = 'Version ' + self._git_version
    else:
      text = 'MRtrix ' + __version__
    text += ' ' * max(1, 40 - len(text) - int(len(self.prog)/2))
    text += bold(self.prog) + '\n'
    if self._is_project:
      text += 'using MRtrix3 ' + __version__ + '\n'
    text += '\n'
    text += '     ' + bold(self.prog) + ': ' + ('external MRtrix3 project' if self._is_project else 'part of the MRtrix3 package') + '\n'
    text += '\n'
    text += bold('SYNOPSIS') + '\n'
    text += '\n'
    text += wrapper_other.fill(self._synopsis) + '\n'
    text += '\n'
    text += bold('USAGE') + '\n'
    text += '\n'
    usage = self.prog + ' [ options ]'
    # Compulsory subparser algorithm selection (if present)
    if self._subparsers:
      usage += ' ' + self._subparsers._group_actions[0].dest + ' ...'
    # Find compulsory input arguments
    for arg in self._positionals._group_actions:
      if arg.metavar:
        usage += ' ' + arg.metavar
      else:
        usage += ' ' + arg.dest
    # Unfortunately this can line wrap early because textwrap is counting each
    #   underlined character as 3 characters when calculating when to wrap
    # Fix by underlining after the fact
    text += wrapper_other.fill(usage).replace(self.prog, underline(self.prog), 1) + '\n'
    text += '\n'
    if self._subparsers:
      text += '        ' + wrapper_args.fill(self._subparsers._group_actions[0].dest + ' '*(max(13-len(self._subparsers._group_actions[0].dest), 1)) + self._subparsers._group_actions[0].help).replace (self._subparsers._group_actions[0].dest, underline(self._subparsers._group_actions[0].dest), 1) + '\n'
      text += '\n'
    for arg in self._positionals._group_actions:
      line = '        '
      if arg.metavar:
        name = arg.metavar
      else:
        name = arg.dest
      line += name + ' '*(max(13-len(name), 1)) + arg.help
      text += wrapper_args.fill(line).replace(name, underline(name), 1) + '\n'
      text += '\n'
    if self._description:
      text += bold('DESCRIPTION') + '\n'
      text += '\n'
      for line in self._description:
        text += wrapper_other.fill(line) + '\n'
        text += '\n'
    if self._examples:
      text += bold('EXAMPLE USAGES') + '\n'
      text += '\n'
      for example in self._examples:
        text += wrapper_other.fill(underline(example[0] + ':')) + '\n'
        text += ' '*7 + '$ ' + example[1] + '\n'
        if example[2]:
          text += wrapper_other.fill(example[2]) + '\n'
        text += '\n'

    # Define a function for printing all text for a given option
    # This will be used in two separate locations:
    #   - First locating and printing any ungrouped command-line options
    #   - Printing all contents of option groups
    def print_group_options(group):
      group_text = ''
      for option in group._group_actions:
        group_text += '  ' + underline('/'.join(option.option_strings))
        if option.metavar:
          group_text += ' '
          if isinstance(option.metavar, tuple):
            group_text += ' '.join(option.metavar)
          else:
            group_text += option.metavar
        elif option.nargs:
          if isinstance(option.nargs, int):
            group_text += (' ' + option.dest.upper())*option.nargs
          elif option.nargs == '+' or option.nargs == '*':
            group_text += ' <space-separated list>'
          elif option.nargs == '?':
            group_text += ' <optional value>'
        elif option.type is not None:
          group_text += ' ' + option.type.__name__.upper()
        elif option.default is None:
          group_text += ' ' + option.dest.upper()
        # Any options that haven't tripped one of the conditions above should be a store_true or store_false, and
        #   therefore there's nothing to be appended to the option instruction
        group_text += '\n'
        group_text += wrapper_other.fill(option.help) + '\n'
        group_text += '\n'
      return group_text

    # Before printing option groups, find any command-line options that have not explicitly been
    #   placed into an option group, and print those first
    ungrouped_options = self._get_ungrouped_options()
    if ungrouped_options and ungrouped_options._group_actions:
      text += bold('OPTIONS') + '\n'
      text += '\n'
      text += print_group_options(ungrouped_options)
    # Option groups
    for group in reversed(self._action_groups):
      if self._is_option_group(group):
        text += bold(group.title) + '\n'
        text += '\n'
        text += print_group_options(group)
    text += bold('AUTHOR') + '\n'
    text += wrapper_other.fill(self._author) + '\n'
    text += '\n'
    text += bold('COPYRIGHT') + '\n'
    text += wrapper_other.fill(self._copyright) + '\n'
    if self._citation_list:
      text += '\n'
      text += bold('REFERENCES') + '\n'
      text += '\n'
      for entry in self._citation_list:
        if entry[0]:
          text += wrapper_other.fill('* ' + entry[0] + ':') + '\n'
        text += wrapper_other.fill(entry[1]) + '\n'
        text += '\n'
    command = CONFIG.get('HelpCommand', 'less -X')
    if command:
      try:
        process = subprocess.Popen(command.split(' '), stdin=subprocess.PIPE)
        process.communicate(text.encode())
      except:
        sys.stdout.write(text)
        sys.stdout.flush()
    else:
      sys.stdout.write(text)
      sys.stdout.flush()

  def print_full_usage(self):
    sys.stdout.write(self._synopsis + '\n')
    if self._description:
      if isinstance(self._description, list):
        for line in self._description:
          sys.stdout.write(line + '\n')
      else:
        sys.stdout.write(self._description + '\n')
    for example in self._examples:
      sys.stdout.write(example[0] + ': $ ' + example[1])
      if example[2]:
        sys.stdout.write('; ' + example[2])
      sys.stdout.write('\n')
    if self._subparsers and len(sys.argv) == 3:
      for alg in self._subparsers._group_actions[0].choices:
        if alg == sys.argv[1]:
          self._subparsers._group_actions[0].choices[alg].print_full_usage()
          return
      self.error('Invalid subparser nominated')
    for arg in self._positionals._group_actions:
      # This will need updating if any scripts allow mulitple argument inputs
      sys.stdout.write('ARGUMENT ' + arg.dest + ' 0 0\n')
      sys.stdout.write(arg.help + '\n')

    def print_group_options(group):
      for option in group._group_actions:
        sys.stdout.write('OPTION ' + '/'.join(option.option_strings) + ' 1 0\n')
        sys.stdout.write(option.help + '\n')
        if option.metavar:
          if isinstance(option.metavar, tuple):
            for arg in option.metavar:
              sys.stdout.write('ARGUMENT ' + arg + ' 0 0\n')
          else:
            sys.stdout.write('ARGUMENT ' + option.metavar + ' 0 0\n')

    ungrouped_options = self._get_ungrouped_options()
    if ungrouped_options and ungrouped_options._group_actions:
      print_group_options(ungrouped_options)
    for group in reversed(self._action_groups):
      if self._is_option_group(group):
        print_group_options(group)
    sys.stdout.flush()

  def print_usage_markdown(self):
    import subprocess
    if self._subparsers and len(sys.argv) == 3:
      for alg in self._subparsers._group_actions[0].choices:
        if alg == sys.argv[-2]:
          self._subparsers._group_actions[0].choices[alg].print_usage_markdown()
          return
      self.error('Invalid subparser nominated')
    text = '## Synopsis\n\n'
    text += self._synopsis + '\n\n'
    text += '## Usage\n\n'
    text += '    ' + self.format_usage() + '\n\n'
    if self._subparsers:
      text += '-  *' + self._subparsers._group_actions[0].dest + '*: ' + self._subparsers._group_actions[0].help + '\n'
    for arg in self._positionals._group_actions:
      if arg.metavar:
        name = arg.metavar
      else:
        name = arg.dest
      text += '-  *' + name + '*: ' + arg.help + '\n\n'
    if self._description:
      text += '## Description\n\n'
      for line in self._description:
        text += line + '\n\n'
    if self._examples:
      text += '## Example usages\n\n'
      for example in self._examples:
        text += '__' + example[0] + ':__\n'
        text += '`$ ' + example[1] + '`\n'
        if example[2]:
          text += example[2] + '\n'
        text += '\n'
    text += '## Options\n\n'

    def print_group_options(group):
      group_text = ''
      for option in group._group_actions:
        option_text = '/'.join(option.option_strings)
        if option.metavar:
          option_text += ' '
          if isinstance(option.metavar, tuple):
            option_text += ' '.join(option.metavar)
          else:
            option_text += option.metavar
        group_text += '+ **-' + option_text + '**<br>' + option.help + '\n\n'
      return group_text

    ungrouped_options = self._get_ungrouped_options()
    if ungrouped_options and ungrouped_options._group_actions:
      text += print_group_options(ungrouped_options)
    for group in reversed(self._action_groups):
      if self._is_option_group(group):
        text += '#### ' + group.title + '\n\n'
        text += print_group_options(group)
    if self._citation_list:
      text += '## References\n\n'
      for ref in self._citation_list:
        ref_text = ''
        if ref[0]:
          ref_text += ref[0] + ': '
        ref_text += ref[1]
        text += ref_text + '\n\n'
    text += '---\n\n'
    text += '**Author:** ' + self._author + '\n\n'
    text += '**Copyright:** ' + self._copyright + '\n\n'
    sys.stdout.write(text)
    sys.stdout.flush()
    if self._subparsers:
      for alg in self._subparsers._group_actions[0].choices:
        subprocess.call ([ sys.executable, os.path.realpath(sys.argv[0]), alg, '__print_usage_markdown__' ])

  def print_usage_rst(self):
    import subprocess
    # Need to check here whether it's the documentation for a particular subparser that's being requested
    if self._subparsers and len(sys.argv) == 3:
      for alg in self._subparsers._group_actions[0].choices:
        if alg == sys.argv[-2]:
          self._subparsers._group_actions[0].choices[alg].print_usage_rst()
          return
      self.error('Invalid subparser nominated: ' + sys.argv[-2])
    text = '.. _' + self.prog.replace(' ', '_') + ':\n\n'
    text += self.prog + '\n'
    text += '='*len(self.prog) + '\n\n'
    text += 'Synopsis\n'
    text += '--------\n\n'
    text += self._synopsis + '\n\n'
    text += 'Usage\n'
    text += '-----\n\n'
    text += '::\n\n'
    text += '    ' + self.format_usage() + '\n\n'
    if self._subparsers:
      text += '-  *' + self._subparsers._group_actions[0].dest + '*: ' + self._subparsers._group_actions[0].help + '\n'
    for arg in self._positionals._group_actions:
      if arg.metavar:
        name = arg.metavar
      else:
        name = arg.dest
      text += '-  *' + name + '*: ' + arg.help.replace('|', '\\|') + '\n'
    text += '\n'
    if self._description:
      text += 'Description\n'
      text += '-----------\n\n'
      for line in self._description:
        text += line + '\n\n'
    if self._examples:
      text += 'Example usages\n'
      text += '--------------\n\n'
      for example in self._examples:
        text += '-   *' + example[0] + '*::\n\n'
        text += '        $ ' + example[1] + '\n\n'
        if example[2]:
          text += '    ' + example[2] + '\n\n'
    text += 'Options\n'
    text += '-------\n'

    def print_group_options(group):
      group_text = ''
      for option in group._group_actions:
        option_text = '/'.join(option.option_strings)
        if option.metavar:
          option_text += ' '
          if isinstance(option.metavar, tuple):
            option_text += ' '.join(option.metavar)
          else:
            option_text += option.metavar
        group_text += '\n'
        group_text += '- **' + option_text + '** ' + option.help.replace('|', '\\|') + '\n'
      return group_text

    ungrouped_options = self._get_ungrouped_options()
    if ungrouped_options and ungrouped_options._group_actions:
      text += print_group_options(ungrouped_options)
    for group in reversed(self._action_groups):
      if self._is_option_group(group):
        text += '\n'
        text += group.title + '\n'
        text += '^'*len(group.title) + '\n'
        text += print_group_options(group)
    if self._citation_list:
      text += '\n'
      text += 'References\n'
      text += '^^^^^^^^^^\n\n'
      for ref in self._citation_list:
        ref_text = '* '
        if ref[0]:
          ref_text += ref[0] + ': '
        ref_text += ref[1]
        text += ref_text + '\n\n'
    else:
      text += '\n'
    text += '--------------\n\n\n\n'
    text += '**Author:** ' + self._author + '\n\n'
    text += '**Copyright:** ' + self._copyright + '\n\n'
    sys.stdout.write(text)
    sys.stdout.flush()
    if self._subparsers:
      for alg in self._subparsers._group_actions[0].choices:
        subprocess.call ([ sys.executable, os.path.realpath(sys.argv[0]), alg, '__print_usage_rst__' ])

  def print_version(self):
    from ._version import __version__
    text = '== ' + self.prog + ' ' + (self._git_version if self._is_project else __version__) + ' ==\n'
    if self._is_project:
      text += 'executing against MRtrix ' + __version__ + '\n'
    text += 'Author(s): ' + self._author + '\n'
    text += self._copyright + '\n'
    sys.stdout.write(text)
    sys.stdout.flush()

  def _get_ungrouped_options(self):
    return next((group for group in self._action_groups if group.title == 'optional arguments'), None)

  def _is_option_group(self, group):
    # * Don't display empty groups
    # * Don't display the subparser option; that's dealt with in the usage
    # * Don't re-display any compulsory positional arguments; they're also dealt with in the usage
    # * Don't display any ungrouped options; those are dealt with explicitly
    return group._group_actions and \
           not (len(group._group_actions) == 1 and \
           isinstance(group._group_actions[0], argparse._SubParsersAction)) and \
           not group == self._positionals and \
           group.title != 'optional arguments'



# Handler function for dealing with system signals
def handler(signum, _frame):
  import shutil, signal
  from mrtrix3 import ANSI
  global _SIGNALS, EXEC_NAME, SCRATCH_DIR, WORKING_DIR
  # Ignore any other incoming signals
  for sig in _SIGNALS:
    try:
      signal.signal(getattr(signal, sig), signal.SIG_IGN)
    except:
      pass
  # Kill any child processes in the run module
  try:
    from mrtrix3.run import shared
    shared.kill()
  except ImportError:
    pass
  # Generate the error message
  msg = '[SYSTEM FATAL CODE: '
  signal_found = False
  for (key, value) in _SIGNALS.items():
    try:
      if getattr(signal, key) == signum:
        msg += key + ' (' + str(int(signum)) + ')] ' + value
        signal_found = True
        break
    except AttributeError:
      pass
  if not signal_found:
    msg += '?] Unknown system signal'
  sys.stderr.write('\n' + EXEC_NAME + ': ' + ANSI.error + msg + ANSI.clear + '\n')
  if os.getcwd() != WORKING_DIR:
    os.chdir(WORKING_DIR)
  if SCRATCH_DIR:
    try:
      shutil.rmtree(SCRATCH_DIR)
    except OSError:
      pass
    SCRATCH_DIR = ''
  sys.exit(signum)
