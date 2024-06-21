# Copyright (c) 2008-2024 the MRtrix3 contributors.
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

import argparse, importlib, inspect, math, os, pathlib, random, shlex, shutil, signal, string, subprocess, sys, textwrap, time
from mrtrix3 import ANSI, CONFIG, MRtrixError, setup_ansi
from mrtrix3 import utils, version



# These global constants can / should be accessed directly by scripts:
# - 'ARGS' will contain the user's command-line inputs upon parsing of the command-line
# - 'CONTINUE_OPTION' will be set to True if the user provides the -continue option;
#   this is principally for use in the run module, and would not typically be accessed within a custom script
# - 'DO_CLEANUP' will indicate whether or not the scratch directory will be deleted on script completion,
#   and whether intermediary files will be deleted when function cleanup() is called on them
# - 'EXEC_NAME' will be the basename of the executed script
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
CONTINUE_OPTION = False
DO_CLEANUP = True
EXEC_NAME = os.path.basename(sys.argv[0])
FORCE_OVERWRITE = False #pylint: disable=unused-variable
NUM_THREADS = None #pylint: disable=unused-variable
SCRATCH_DIR = ''
VERBOSITY = 0 if 'MRTRIX_QUIET' in os.environ else int(os.environ.get('MRTRIX_LOGLEVEL', '1'))
WORKING_DIR = os.getcwd()



# - 'CMDLINE' needs to be updated with any compulsory arguments and optional command-line inputs
#   necessary for the executable script to be added via its usage() function
#   It will however be passed to the calling executable as a parameter in the usage() function,
#   and should not be modified outside of this module outside of such functions
CMDLINE = None



_DEFAULT_COPYRIGHT = \
'''Copyright (c) 2008-2024 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.

Covered Software is provided under this License on an "as is"
basis, without warranty of any kind, either expressed, implied, or
statutory, including, without limitation, warranties that the
Covered Software is free of defects, merchantable, fit for a
particular purpose or non-infringing.
See the Mozilla Public License v. 2.0 for more details.

For more details, see http://www.mrtrix.org/.'''



_MRTRIX3_CORE_REFERENCE = 'Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. \
MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. \
NeuroImage, 2019, 202, 116137'



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
             'SIGXCPU': 'CPU time limit exceeded',
             'SIGXFSZ': 'File size limit exceeded' }
           # Can't be handled; see https://bugs.python.org/issue9524
           # 'CTRL_C_EVENT': 'Terminated by user Ctrl-C input',
           # 'CTRL_BREAK_EVENT': 'Terminated by user Ctrl-Break input'
if utils.is_windows():
  _SIGNALS['SIGBREAK'] = 'Received Windows \'break\' signal'
else:
  _SIGNALS['SIGTERM'] = 'Received termination signal'



# Store any input piped images that need to be deleted upon script completion
#   rather than when some underlying MRtrix3 command reads them
_STDIN_IMAGES = []
# Store output piped images that need to be emitted to stdout upon script completion
_STDOUT_IMAGES = []



# This function gets executed by the corresponding cmake-generated Python executable
def _execute(usage_function, execute_function): #pylint: disable=unused-variable
  from mrtrix3 import run #pylint: disable=import-outside-toplevel
  global ARGS, CMDLINE, CONTINUE_OPTION, DO_CLEANUP, FORCE_OVERWRITE, NUM_THREADS, SCRATCH_DIR, VERBOSITY

  assert inspect.isfunction(usage_function) and inspect.isfunction(execute_function)

  # Set up signal handlers
  for sig in _SIGNALS:
    try:
      signal.signal(getattr(signal, sig), handler)
    except AttributeError:
      pass

  CMDLINE = Parser()
  usage_function(CMDLINE)

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
  if hasattr(ARGS, 'nthreads') and ARGS.nthreads is not None:
    NUM_THREADS = ARGS.nthreads #pylint: disable=unused-variable
  if hasattr(ARGS, 'quiet') and ARGS.quiet:
    VERBOSITY = 0
  elif hasattr(ARGS, 'info') and ARGS.info:
    VERBOSITY = 2
  elif hasattr(ARGS, 'debug') and ARGS.debug:
    VERBOSITY = 3

  if hasattr(ARGS, 'config') and ARGS.config:
    for keyval in ARGS.config:
      CONFIG[keyval[0]] = keyval[1]

  # Now that FORCE_OVERWRITE has been set,
  #   check any user-specified output paths
  try:
    for key in vars(ARGS):
      value = getattr(ARGS, key)
      if isinstance(value, Parser._UserOutPathExtras): # pylint: disable=protected-access
        value.check_output()
  except FileExistsError as exception:
    sys.stderr.write('\n')
    sys.stderr.write(f'{EXEC_NAME}: {ANSI.error}[ERROR] {exception}{ANSI.clear}\n')
    sys.stderr.flush()
    sys.exit(1)

  # ANSI settings may have been altered at the command-line
  setup_ansi()

  # Check compatibility with command-line piping
  # if _STDIN_IMAGES and sys.stdin.isatty():
  #   sys.stderr.write(f{EXEC_NAME}: {ANSI.error}[ERROR] Piped input images not available from stdin{ANSI.clear}\n')
  #   sys.stderr.flush()
  #   sys.exit(1)
  if _STDOUT_IMAGES and sys.stdout.isatty():
    sys.stderr.write(f'{EXEC_NAME}: {ANSI.error}[ERROR] Cannot pipe output images as no command connected to stdout{ANSI.clear}\n')
    sys.stderr.flush()
    sys.exit(1)

  if hasattr(ARGS, 'cont') and ARGS.cont:
    CONTINUE_OPTION = True
    SCRATCH_DIR = os.path.abspath(ARGS.cont[0])
    try:
      os.remove(os.path.join(SCRATCH_DIR, 'error.txt'))
    except OSError:
      pass
    run.shared.set_continue(ARGS.cont[1])

  run.shared.set_verbosity(VERBOSITY)
  run.shared.set_num_threads(NUM_THREADS)

  CMDLINE.print_citation_warning()

  return_code = 0

  cli_parse_only = os.getenv('MRTRIX_CLI_PARSE_ONLY')
  if cli_parse_only:
    try:
      if cli_parse_only.lower() in ['yes', 'true'] or int(cli_parse_only):
        console(
          'Quitting after parsing command-line arguments successfully due to '
          'environment variable "MRTRIX_CLI_PARSE_ONLY"'
        )
        sys.exit(return_code)
    except ValueError:
      warn('Potentially corrupt environment variable "MRTRIX_CLI_PARSE_ONLY" '
           '= "' + cli_parse_only + '"; ignoring')
    sys.exit(return_code)

  try:
    execute_function()
  except (run.MRtrixCmdError, run.MRtrixFnError) as exception:
    is_cmd = isinstance(exception, run.MRtrixCmdError)
    return_code = exception.returncode if is_cmd else 1
    DO_CLEANUP = False
    if SCRATCH_DIR:
      with open(os.path.join(SCRATCH_DIR, 'error.txt'), 'w', encoding='utf-8') as outfile:
        outfile.write((exception.command if is_cmd else exception.function) + '\n\n' + str(exception) + '\n')
    exception_frame = inspect.getinnerframes(sys.exc_info()[2])[-2]
    try:
      filename = exception_frame.filename
      lineno = exception_frame.lineno
    except AttributeError: # Prior to Python 3.5
      filename = exception_frame[1]
      lineno = exception_frame[2]
    sys.stderr.write('\n')
    sys.stderr.write(f'{EXEC_NAME}: {ANSI.error}[ERROR] {exception.command if is_cmd else exception.function}{ANSI.clear} {ANSI.debug}({os.path.basename(filename)}:{lineno}){ANSI.clear}\n')
    if str(exception):
      sys.stderr.write(f'{EXEC_NAME}: {ANSI.error}[ERROR] Information from failed {"command" if is_cmd else "function"}:{ANSI.clear}\n')
      sys.stderr.write(f'{EXEC_NAME}:\n')
      for line in str(exception).splitlines():
        sys.stderr.write(f'{" " * (len(EXEC_NAME)+2)}{line}\n')
      sys.stderr.write(f'{EXEC_NAME}:\n')
    else:
      sys.stderr.write(f'{EXEC_NAME}: {ANSI.error}[ERROR] Failed {"command" if is_cmd else "function"} did not provide any output information{ANSI.clear}\n')
    if SCRATCH_DIR:
      sys.stderr.write(f'{EXEC_NAME}: {ANSI.error}[ERROR] For debugging, inspect contents of scratch directory: {SCRATCH_DIR}{ANSI.clear}\n')
    sys.stderr.flush()
  except MRtrixError as exception:
    return_code = 1
    sys.stderr.write('\n')
    sys.stderr.write(f'{EXEC_NAME}: {ANSI.error}[ERROR] {exception}{ANSI.clear}\n')
    sys.stderr.flush()
  except Exception as exception: # pylint: disable=broad-except
    return_code = 1
    sys.stderr.write('\n')
    sys.stderr.write(f'{EXEC_NAME}: {ANSI.error}[ERROR] Unhandled Python exception:{ANSI.clear}\n')
    sys.stderr.write(f'{EXEC_NAME}: {ANSI.error}[ERROR]{ANSI.clear}   {ANSI.console}{type(exception).__name__}: {exception}{ANSI.clear}\n')
    traceback = sys.exc_info()[2]
    sys.stderr.write(f'{EXEC_NAME}: {ANSI.error}[ERROR] Traceback:{ANSI.clear}\n')
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
      sys.stderr.write(f'{EXEC_NAME}: {ANSI.error}[ERROR]{ANSI.clear}   {ANSI.console}{filename}:{lineno} (in {function}()){ANSI.clear}\n')
      for line in calling_code:
        sys.stderr.write(f'{EXEC_NAME}: {ANSI.error}[ERROR]{ANSI.clear}     {ANSI.debug}{line.strip()}{ANSI.clear}\n')
  finally:
    if os.getcwd() != WORKING_DIR:
      if not return_code:
        console(f'Changing back to original directory ({WORKING_DIR})')
      os.chdir(WORKING_DIR)
    if _STDIN_IMAGES:
      debug(f'Erasing {len(_STDIN_IMAGES)} piped input images')
      for item in _STDIN_IMAGES:
        try:
          item.unlink()
          debug(f'Successfully erased "{item}"')
        except FileNotFoundError as exc:
          debug(f'Unable to erase "{item}": {exc}')
    if SCRATCH_DIR:
      if DO_CLEANUP:
        if not return_code:
          console(f'Deleting scratch directory ({SCRATCH_DIR})')
        try:
          shutil.rmtree(SCRATCH_DIR)
        except OSError:
          pass
        SCRATCH_DIR = ''
      else:
        console(f'Scratch directory retained; location: {SCRATCH_DIR}')
    if _STDOUT_IMAGES:
      debug(f'Emitting {len(_STDOUT_IMAGES)} output piped images to stdout')
      sys.stdout.write('\n'.join(map(str, _STDOUT_IMAGES)))
  sys.exit(return_code)



def activate_scratch_dir(): #pylint: disable=unused-variable
  from mrtrix3 import run #pylint: disable=import-outside-toplevel
  global SCRATCH_DIR
  if CONTINUE_OPTION:
    debug('Skipping scratch directory creation due to use of -continue option')
    return
  if SCRATCH_DIR:
    raise Exception('Cannot use multiple scratch directories')
  if hasattr(ARGS, 'scratch') and ARGS.scratch:
    dir_path = ARGS.scratch
  else:
    # Defaulting to working directory since too many users have encountered storage issues
    dir_path = CONFIG.get('ScriptScratchDir', WORKING_DIR)
  prefix = CONFIG.get('ScriptScratchPrefix', f'{EXEC_NAME}-tmp-')
  SCRATCH_DIR = dir_path
  while os.path.isdir(SCRATCH_DIR):
    random_string = ''.join(random.choice(string.ascii_uppercase + string.digits) for x in range(6))
    SCRATCH_DIR = os.path.join(dir_path, f'{prefix}{random_string}') + os.sep
  os.makedirs(SCRATCH_DIR)
  os.chdir(SCRATCH_DIR)
  if VERBOSITY:
    console(f'Activated scratch directory: {SCRATCH_DIR}')
  with open('cwd.txt', 'w', encoding='utf-8') as outfile:
    outfile.write(f'{WORKING_DIR}\n')
  with open('command.txt', 'w', encoding='utf-8') as outfile:
    outfile.write(f'{" ".join(sys.argv)}\n')
  with open('log.txt', 'w', encoding='utf-8'):
    pass
  # Also use this scratch directory for any piped images within run.command() calls,
  #   and for keeping a log of executed commands / functions
  run.shared.set_scratch_dir(SCRATCH_DIR)



# This function can (and should in some instances) be called upon any file / directory
#   that is no longer required by the script. If the script has been instructed to retain
#   all intermediates, the resource will be retained; if not, it will be deleted (in particular
#   to dynamically free up storage space used by the script).
def cleanup(items): #pylint: disable=unused-variable
  if not DO_CLEANUP or not items:
    return
  if isinstance(items, list):
    if len(items) == 1:
      cleanup(items[0])
      return
    if VERBOSITY > 2:
      console(f'Cleaning up {len(items)} intermediate items: {items}')
    for item in items:
      if os.path.isfile(item):
        func = os.remove
      elif os.path.isdir(item):
        func = shutil.rmtree
      else:
        continue
      try:
        func(item)
      except OSError:
        pass
    return
  item = items
  if os.path.isfile(item):
    item_type = 'file'
    func = os.remove
  elif os.path.isdir(item):
    item_type = 'directory'
    func = shutil.rmtree
  else:
    debug(f'Unknown target "{item}"')
    return
  if VERBOSITY > 2:
    console(f'Cleaning up intermediate {item_type}: "{item}"')
  try:
    func(item)
  except OSError:
    debug(f'Unable to cleanup intermediate {item_type}: "{item}"')






# A set of functions and variables for printing various information at the command-line.
def console(text): #pylint: disable=unused-variable
  if VERBOSITY:
    sys.stderr.write(f'{EXEC_NAME}: {ANSI.console}{text}{ANSI.clear}\n')

def debug(text): #pylint: disable=unused-variable
  if VERBOSITY <= 2:
    return
  outer_frames = inspect.getouterframes(inspect.currentframe())
  nearest = outer_frames[1]
  try:
    if len(outer_frames) == 2: # debug() called directly from script being executed
      origin = f'({os.path.basename(nearest.filename)}:{nearest.lineno})'
    else: # Some function has called debug(): Get location of both that function, and where that function was invoked
      filename = nearest.filename
      funcname = f'{nearest.function}()'
      modulename = inspect.getmodulename(filename)
      if modulename:
        funcname = f'{modulename}.{funcname}'
      origin = funcname
      caller = outer_frames[2]
      origin += f' (from {os.path.basename(caller.filename)}:{caller.lineno})'
    sys.stderr.write(f'{EXEC_NAME}: {ANSI.debug}[DEBUG] {origin}: {text}{ANSI.clear}\n')
  finally:
    del nearest

def trace(): #pylint: disable=unused-variable
  calling_frame = inspect.getouterframes(inspect.currentframe())[1]
  try:
    filename = calling_frame.filename
    lineno = calling_frame.lineno
    sys.stderr.write(f'{EXEC_NAME}: at {os.path.basename(filename)}:{lineno}\n')
  finally:
    del calling_frame

def var(*variables): #pylint: disable=unused-variable
  calling_frame = inspect.getouterframes(inspect.currentframe())[1]
  try:
    calling_code = calling_frame.code_context[0]
    filename = calling_frame.filename
    lineno = calling_frame.lineno
    var_string = calling_code[calling_code.find('var(')+4:].rstrip('\n').rstrip(' ')[:-1].replace(',', ' ')
    var_names, var_values = var_string.split(), variables
    for name, value in zip(var_names, var_values):
      sys.stderr.write(f'{EXEC_NAME}: [{os.path.basename(filename)}:{lineno}]: {name} = {value}\n')
  finally:
    del calling_frame

def warn(text): #pylint: disable=unused-variable
  sys.stderr.write(f'{EXEC_NAME}: {ANSI.warn}[WARNING] {text}{ANSI.clear}\n')



# A class that can be used to display a progress bar on the terminal,
#   mimicing the behaviour of MRtrix3 binary commands
class ProgressBar: #pylint: disable=unused-variable

  BUSY = [ '.   ',
           ' .  ',
           '  . ',
           '   .',
           '  . ',
           ' .  ' ]

  INTERVAL = 0.1
  WRAPON = '\033[?7h'
  WRAPOFF = '\033[?7l'

  def __init__(self, msg, target=0):
    from mrtrix3 import run #pylint: disable=import-outside-toplevel
    global VERBOSITY
    if not (isinstance(msg, str) or callable(msg)):
      raise TypeError('app.ProgressBar must be constructed using either a string or a function')
    self.counter = 0
    self.isatty = sys.stderr.isatty()
    self.iscomplete = False
    self.message = msg
    self.multiplier = 100.0/target if target else 0
    self.newline = '\n' if VERBOSITY > 1 else '' # If any more than default verbosity, may still get details printed in between progress updates
    self.next_time = time.time() + ProgressBar.INTERVAL
    self.old_value = 0
    self.orig_verbosity = VERBOSITY
    self.value = 0
    # Only disable wrapping if the progress bar is the only thing being printed
    self.wrapoff = '' if self.newline else ProgressBar.WRAPOFF
    self.wrapon = '' if self.newline else ProgressBar.WRAPON
    VERBOSITY = run.shared.verbosity = VERBOSITY - 1 if VERBOSITY else 0
    if not self.orig_verbosity:
      return
    if self.isatty:
      progress_bar = f'{self.value:>3}%' if self.multiplier else ProgressBar.BUSY[0]
      sys.stderr.write(f'{self.wrapoff}{EXEC_NAME}: {ANSI.execute}[{progress_bar}]{ANSI.clear} {ANSI.console}{self._get_message()}... {ANSI.clear}{ANSI.lineclear}{self.wrapon}{self.newline}')
    else:
      sys.stderr.write(f'{EXEC_NAME}: {self._get_message()}... [{self.newline}')
    sys.stderr.flush()

  def increment(self, msg=None):
    assert not self.iscomplete
    self.counter += 1
    force_update = False
    if msg is not None:
      self.message = msg
      force_update = True
    if self.multiplier:
      new_value = int(round(self.counter * self.multiplier))
    elif self.isatty:
      new_value = self.counter
    else:
      new_value = int(round(math.log(self.counter, 2))) + 1
    if new_value != self.value:
      self.old_value = self.value
      self.value = new_value
      force_update = True
    if force_update:
      current_time = time.time()
      if current_time >= self.next_time:
        self.next_time = current_time + ProgressBar.INTERVAL
        self._update()

  def done(self, msg=None):
    from mrtrix3 import run #pylint: disable=import-outside-toplevel
    global VERBOSITY
    self.iscomplete = True
    if msg is not None:
      self.message = msg
    if self.multiplier:
      self.value = 100
    VERBOSITY = run.shared.verbosity = self.orig_verbosity
    if not self.orig_verbosity:
      return
    if self.isatty:
      progress_bar = '100%' if self.multiplier else 'done'
      sys.stderr.write(f'\r{EXEC_NAME}: {ANSI.execute}[{progress_bar}]{ANSI.clear} {ANSI.console}{self._get_message()}{ANSI.clear}{ANSI.lineclear}\n')
    else:
      if self.newline:
        sys.stderr.write(f'{EXEC_NAME}: {self._get_message()} [{"=" * int(self.value/2)}]\n')
      else:
        sys.stderr.write(f'{"=" * (int(self.value/2) - int(self.old_value/2))}]\n')
    sys.stderr.flush()


  def _update(self):
    assert not self.iscomplete
    if not self.orig_verbosity:
      return
    if self.isatty:
      progress_bar = f'{self.value:>3}%' if self.multiplier else ProgressBar.BUSY[self.counter%6]
      sys.stderr.write(f'{self.wrapoff}\r{EXEC_NAME}: {ANSI.execute}[{progress_bar}]{ANSI.clear} {ANSI.console}{self._get_message()}... {ANSI.clear}{ANSI.lineclear}{self.wrapon}{self.newline}')
    else:
      if self.newline:
        sys.stderr.write(f'{EXEC_NAME}: {self._get_message()}... [{"=" * int(self.value/2)}{self.newline}')
      else:
        sys.stderr.write('=' * (int(self.value/2) - int(self.old_value/2)))
    sys.stderr.flush()

  def _get_message(self):
    return self.message() if callable(self.message) else self.message













# The Parser class is responsible for setting up command-line parsing for the script.
#   This includes proper configuration of the argparse functionality, adding standard options
#   that are common for all scripts, providing a custom help page that is consistent with the
#   MRtrix3 binaries, and defining functions for exporting the help page for the purpose of
#   automated self-documentation.

class Parser(argparse.ArgumentParser):

  # Function that will create a new class,
  #   which will derive from both pathlib.Path (which itself through __new__() could be Posix or Windows)
  #   and a desired augmentation that provides additional functions
  @staticmethod
  def make_userpath_object(base_class, *args, **kwargs):
    abspath = os.path.normpath(os.path.join(WORKING_DIR, *args))
    new_class = type(f'{base_class.__name__.lstrip("_").rstrip("Extras")}',
                    (base_class,
                     pathlib.WindowsPath if os.name == 'nt' else pathlib.PosixPath),
                    {})
    instance = new_class.__new__(new_class, abspath, **kwargs)
    return instance

  # Classes that extend the functionality of pathlib.Path
  class _UserPathExtras:
    def __format__(self, _):
      return shlex.quote(str(self))
  class _UserOutPathExtras(_UserPathExtras):
    def __init__(self, *args, **kwargs):
      super().__init__(self, *args, **kwargs)
    def check_output(self, item_type='path'):
      if self.exists(): # pylint: disable=no-member
        if FORCE_OVERWRITE:
          warn(f'Output {item_type} "{str(self)}" already exists; '
                'will be overwritten at script completion')
        else:
          raise FileExistsError(f'Output {item_type} "{str(self)}" already exists '
                            '(use -force option to force overwrite)')
  class _UserFileOutPathExtras(_UserOutPathExtras):
    def __init__(self, *args, **kwargs):
      super().__init__(self, *args, **kwargs)
    def check_output(self): # pylint: disable=arguments-differ
      return super().check_output('file')
  class _UserDirOutPathExtras(_UserOutPathExtras):
    def __init__(self, *args, **kwargs):
      super().__init__(self, *args, **kwargs)
    def check_output(self): # pylint: disable=arguments-differ
      return super().check_output('directory')
    # Force parents=True for user-specified path
    # Force exist_ok=False for user-specified path
    def mkdir(self, mode=0o777): # pylint: disable=arguments-differ
      while True:
        if FORCE_OVERWRITE:
          try:
            shutil.rmtree(self)
          except FileNotFoundError:
            pass
        try:
          super().mkdir(mode, parents=True, exist_ok=False) # pylint: disable=no-member
          return
        except FileExistsError:
          if not FORCE_OVERWRITE:
            # pylint: disable=raise-missing-from
            raise FileExistsError(f'Output directory "{str(self)}" already exists '
                                  '(use -force option to force overwrite)')

  # Various callable types for use as argparse argument types
  class CustomTypeBase:
    @staticmethod
    def _legacytypestring():
      assert False
    @staticmethod
    def _metavar():
      assert False

  class Bool(CustomTypeBase):
    def __call__(self, input_value):
      processed_value = input_value.strip().lower()
      if processed_value in ['true', 'yes']:
        return True
      if processed_value in ['false', 'no']:
        return False
      try:
        processed_value = int(processed_value)
      except ValueError as exc:
        raise argparse.ArgumentTypeError(f'Could not interpret "{input_value}" as boolean value') from exc
      return bool(processed_value)
    @staticmethod
    def _legacytypestring():
      return 'BOOL'
    @staticmethod
    def _metavar():
      return 'value'

  def Int(min_value=None, max_value=None): # pylint: disable=invalid-name,no-self-argument
    assert min_value is None or isinstance(min_value, int)
    assert max_value is None or isinstance(max_value, int)
    assert min_value is None or max_value is None or max_value >= min_value
    class IntBounded(Parser.CustomTypeBase):
      def __call__(self, input_value):
        try:
          value = int(input_value)
        except ValueError as exc:
          raise argparse.ArgumentTypeError(f'Could not interpret "{input_value}" as integer value') from exc
        if min_value is not None and value < min_value:
          raise argparse.ArgumentTypeError(f'Input value "{input_value}" less than minimum permissible value {min_value}')
        if max_value is not None and value > max_value:
          raise argparse.ArgumentTypeError(f'Input value "{input_value}" greater than maximum permissible value {max_value}')
        return value
      @staticmethod
      def _legacytypestring():
        return f'INT {-sys.maxsize - 1 if min_value is None else min_value} {sys.maxsize if max_value is None else max_value}'
      @staticmethod
      def _metavar():
        return 'value'
    return IntBounded()

  def Float(min_value=None, max_value=None): # pylint: disable=invalid-name,no-self-argument
    assert min_value is None or isinstance(min_value, float)
    assert max_value is None or isinstance(max_value, float)
    assert min_value is None or max_value is None or max_value >= min_value
    class FloatBounded(Parser.CustomTypeBase):
      def __call__(self, input_value):
        try:
          value = float(input_value)
        except ValueError as exc:
          raise argparse.ArgumentTypeError(f'Could not interpret "{input_value}" as floating-point value') from exc
        if min_value is not None and value < min_value:
          raise argparse.ArgumentTypeError(f'Input value "{input_value}" less than minimum permissible value {min_value}')
        if max_value is not None and value > max_value:
          raise argparse.ArgumentTypeError(f'Input value "{input_value}" greater than maximum permissible value {max_value}')
        return value
      @staticmethod
      def _legacytypestring():
        return f'FLOAT {"-inf" if min_value is None else str(min_value)} {"inf" if max_value is None else str(max_value)}'
      @staticmethod
      def _metavar():
        return 'value'
    return FloatBounded()

  class SequenceInt(CustomTypeBase):
    def __call__(self, input_value):
      try:
        return [int(i) for i in input_value.split(',')]
      except ValueError as exc:
        raise argparse.ArgumentTypeError(f'Could not interpret "{input_value}" as integer sequence') from exc
    @staticmethod
    def _legacytypestring():
      return 'ISEQ'
    @staticmethod
    def _metavar():
      return 'values'

  class SequenceFloat(CustomTypeBase):
    def __call__(self, input_value):
      try:
        return [float(i) for i in input_value.split(',')]
      except ValueError as exc:
        raise argparse.ArgumentTypeError(f'Could not interpret "{input_value}" as floating-point sequence') from exc
    @staticmethod
    def _legacytypestring():
      return 'FSEQ'
    @staticmethod
    def _metavar():
      return 'values'

  class DirectoryIn(CustomTypeBase):
    def __call__(self, input_value):
      abspath = Parser.make_userpath_object(Parser._UserPathExtras, input_value)
      if not abspath.exists():
        raise argparse.ArgumentTypeError(f'Input directory "{input_value}" does not exist')
      if not abspath.is_dir():
        raise argparse.ArgumentTypeError(f'Input path "{input_value}" is not a directory')
      return abspath
    @staticmethod
    def _legacytypestring():
      return 'DIRIN'
    @staticmethod
    def _metavar():
      return 'directory'

  class DirectoryOut(CustomTypeBase):
    def __call__(self, input_value):
      abspath = Parser.make_userpath_object(Parser._UserDirOutPathExtras, input_value)
      return abspath
    @staticmethod
    def _legacytypestring():
      return 'DIROUT'
    @staticmethod
    def _metavar():
      return 'directory'

  class FileIn(CustomTypeBase):
    def __call__(self, input_value):
      abspath = Parser.make_userpath_object(Parser._UserPathExtras, input_value)
      if not abspath.exists():
        raise argparse.ArgumentTypeError(f'Input file "{input_value}" does not exist')
      if not abspath.is_file():
        raise argparse.ArgumentTypeError(f'Input path "{input_value}" is not a file')
      return abspath
    @staticmethod
    def _legacytypestring():
      return 'FILEIN'
    @staticmethod
    def _metavar():
      return 'file'

  class FileOut(CustomTypeBase):
    def __call__(self, input_value):
      return Parser.make_userpath_object(Parser._UserFileOutPathExtras, input_value)
    @staticmethod
    def _legacytypestring():
      return 'FILEOUT'
    @staticmethod
    def _metavar():
      return 'file'

  class ImageIn(CustomTypeBase):
    def __call__(self, input_value):
      if input_value == '-':
        input_value = sys.stdin.readline().strip()
        abspath = pathlib.Path(input_value)
        _STDIN_IMAGES.append(abspath)
        return abspath
      return Parser.make_userpath_object(Parser._UserPathExtras, input_value)
    @staticmethod
    def _legacytypestring():
      return 'IMAGEIN'
    @staticmethod
    def _metavar():
      return 'image'

  class ImageOut(CustomTypeBase):
    def __call__(self, input_value):
      if input_value == '-':
        input_value = utils.name_temporary('mif')
        abspath = pathlib.Path(input_value)
        _STDOUT_IMAGES.append(abspath)
        return abspath
      # Not guaranteed to catch all cases of output images trying to overwrite existing files;
      #   but will at least catch some of them
      return Parser.make_userpath_object(Parser._UserFileOutPathExtras, input_value)
    @staticmethod
    def _legacytypestring():
      return 'IMAGEOUT'
    @staticmethod
    def _metavar():
      return 'image'

  class TracksIn(CustomTypeBase):
    def __call__(self, input_value):
      filepath = Parser.FileIn()(input_value)
      if filepath.suffix.lower() != '.tck':
        raise argparse.ArgumentTypeError(f'Input tractogram file "{filepath}" is not a valid track file')
      return filepath
    @staticmethod
    def _legacytypestring():
      return 'TRACKSIN'
    @staticmethod
    def _metavar():
      return 'trackfile'

  class TracksOut(CustomTypeBase):
    def __call__(self, input_value):
      filepath = Parser.FileOut()(input_value)
      if filepath.suffix.lower() != '.tck':
        raise argparse.ArgumentTypeError(f'Output tractogram path "{filepath}" does not use the requisite ".tck" suffix')
      return filepath
    @staticmethod
    def _legacytypestring():
      return 'TRACKSOUT'
    @staticmethod
    def _metavar():
      return 'trackfile'

  class Various(CustomTypeBase):
    def __call__(self, input_value):
      return input_value
    @staticmethod
    def _legacytypestring():
      return 'VARIOUS'
    @staticmethod
    def _metavar():
      return 'spec'





  # pylint: disable=protected-access
  def __init__(self, *args_in, **kwargs_in):
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
      standard_options.add_argument('-info',
                                    action='store_true',
                                    default=None,
                                    help='display information messages.')
      standard_options.add_argument('-quiet',
                                    action='store_true',
                                    default=None,
                                    help='do not display information messages or progress status. '
                                         'Alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.')
      standard_options.add_argument('-debug',
                                    action='store_true',
                                    default=None,
                                    help='display debugging messages.')
      self.flag_mutually_exclusive_options( [ 'info', 'quiet', 'debug' ] )
      standard_options.add_argument('-force',
                                    action='store_true',
                                    default=None,
                                    help='force overwrite of output files.')
      standard_options.add_argument('-nthreads',
                                    metavar='number',
                                    type=Parser.Int(0),
                                    help='use this number of threads in multi-threaded applications '
                                         '(set to 0 to disable multi-threading).')
      standard_options.add_argument('-config',
                                    action='append',
                                    type=str,
                                    metavar=('key', 'value'),
                                    nargs=2,
                                    help='temporarily set the value of an MRtrix config file entry.')
      standard_options.add_argument('-help',
                                    action='store_true',
                                    default=None,
                                    help='display this information page and exit.')
      standard_options.add_argument('-version',
                                    action='store_true',
                                    default=None,
                                    help='display version information and exit.')
      script_options = self.add_argument_group('Additional standard options for Python scripts')
      script_options.add_argument('-nocleanup',
                                  action='store_true',
                                  default=None,
                                  help='do not delete intermediate files during script execution, '
                                       'and do not delete scratch directory at script completion.')
      script_options.add_argument('-scratch',
                                  type=Parser.DirectoryIn(),
                                  metavar='/path/to/scratch/',
                                  help='manually specify an existing directory in which to generate the scratch directory.')
      script_options.add_argument('-continue',
                                  type=Parser.Various(),
                                  nargs=2,
                                  dest='cont',
                                  metavar=('ScratchDir', 'LastFile'),
                                  help='continue the script from a previous execution; '
                                       'must provide the scratch directory path, '
                                       'and the name of the last successfully-generated file.')
    module_file = os.path.realpath (inspect.getsourcefile(inspect.stack()[-1][0]))
    self._is_project = os.path.abspath(os.path.join(os.path.dirname(module_file), os.pardir, 'lib', 'mrtrix3', 'app.py')) != os.path.abspath(__file__)
    try:
      with subprocess.Popen ([ 'git', 'describe', '--abbrev=8', '--dirty', '--always' ],
                             cwd=os.path.abspath(os.path.join(os.path.dirname(module_file), os.pardir)),
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE) as process:
        self._git_version = process.communicate()[0]
        self._git_version = str(self._git_version.decode(errors='ignore')).strip() \
                            if process.returncode == 0 \
                            else 'unknown'
    except OSError:
      self._git_version = 'unknown'

  def set_author(self, text):
    self._author = text

  def set_synopsis(self, text):
    self._synopsis = text

  def add_citation(self, citation, **kwargs): #pylint: disable=unused-variable
    # condition, is_external
    condition = kwargs.pop('condition', None)
    is_external = kwargs.pop('is_external', False)
    if kwargs:
      raise TypeError('Unsupported keyword arguments passed to app.Parser.add_citation(): '
                      + str(kwargs))
    self._citation_list.append( (condition, citation) )
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

  def add_subparsers(self): # pylint: disable=arguments-differ
    # Import the command-line settings for all algorithms in the relevant sub-directories
    # This is expected to be being called from the 'usage' module of the relevant command
    module_name = os.path.dirname(inspect.getouterframes(inspect.currentframe())[1].filename).split(os.sep)[-1]
    module = sys.modules['mrtrix3.commands.' + module_name]
    base_parser = Parser(description='Base parser for construction of subparsers', parents=[self])
    subparsers = super().add_subparsers(title='Algorithm choices',
                                        help='Select the algorithm to be used; '
                                             'additional details and options become available once an algorithm is nominated. '
                                             'Options are: ' + ', '.join(module.ALGORITHMS),
                                        dest='algorithm')
    for algorithm in module.ALGORITHMS:
      algorithm_module = importlib.import_module('.' + algorithm, 'mrtrix3.commands.' + module_name)
      algorithm_module.usage(base_parser, subparsers)

  def parse_args(self, args=None, namespace=None):
    if not self._author:
      raise Exception('Script author MUST be set in script\'s usage() function')
    if not self._synopsis:
      raise Exception('Script synopsis MUST be set in script\'s usage() function')
    if '-version' in args if args else '-version' in sys.argv[1:]:
      self.print_version()
      sys.exit(0)
    result = super().parse_args(args, namespace)
    self._check_mutex_options(result)
    if self._subparsers:
      for alg in self._subparsers._group_actions[0].choices:
        self._subparsers._group_actions[0].choices[alg]._check_mutex_options(result)
    return result

  def print_citation_warning(self):
    # If a subparser has been invoked, the subparser's function should instead be called,
    #   since it might have had additional citations appended
    if self._subparsers:
      subparser = getattr(ARGS, self._subparsers._group_actions[0].dest)
      for alg in self._subparsers._group_actions[0].choices:
        if alg == subparser:
          self._subparsers._group_actions[0].choices[alg].print_citation_warning()
          return
    if not self._external_citations:
      return
    console('')
    console('Note that this script may make use of commands / algorithms'
            ' from neuroimaging software other than MRtrix3.')
    console('PLEASE ENSURE that any non-MRtrix3 software,'
            ' as well as any research methods they provide,'
            ' are recognised and cited appropriately.')
    console('Consult the help page (-help option) for more information.')
    console('')

  # Overloads argparse.ArgumentParser function to give a better error message on failed parsing
  def error(self, message):
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
    sys.stderr.write(f'\nError: {message}\n')
    sys.stderr.write(f'Usage: {usage}\n')
    sys.stderr.write(f'       (Run {self.prog} -help for more information)\n\n')
    sys.stderr.flush()
    sys.exit(2)

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
        sys.stderr.write(f'\nError: You cannot use more than one of the following options: {", ".join([ "-" + o for o in group[0] ])}\n')
        sys.stderr.write(f'(Consult the help page for more information: {self.prog} -help)\n\n')
        sys.stderr.flush()
        sys.exit(1)
      if group[1] and not count:
        sys.stderr.write(f'\nError: One of the following options must be provided: {", ".join([ "-" + o for o in group[0] ])}\n')
        sys.stderr.write(f'(Consult the help page for more information: {self.prog} -help)\n\n')
        sys.stderr.flush()
        sys.exit(1)

  @staticmethod
  def _option2metavar(option):
    if option.metavar is not None:
      if isinstance(option.metavar, tuple):
        return f' {" ".join(option.metavar)}'
      text = option.metavar
    elif option.choices is not None:
      return ' choice'
    elif isinstance(option.type, Parser.CustomTypeBase):
      text = option.type._metavar()
    elif option.type is not None:
      text = option.type.__name__.lower()
    elif option.nargs == 0:
      return ''
    else:
      text = 'string'
    if option.nargs:
      if isinstance(option.nargs, int) and option.nargs > 1:
        text = ((f' {text}') * option.nargs).lstrip()
      elif option.nargs == '*':
        text = f'<space-separated list of {text}s>'
      elif option.nargs == '+':
        text = f'{text} <space-separated list of additional {text}s>'
      elif option.nargs == '?':
        text = f'<optional {text}>'
    return f' {text}'

  def format_usage(self):
    argument_list = [ ]
    trailing_ellipsis = ''
    if self._subparsers:
      argument_list.append(self._subparsers._group_actions[0].dest)
      trailing_ellipsis = ' ...'
    for arg in self._positionals._group_actions:
      if arg.metavar:
        argument_list.append(' '.join(arg.metavar))
      else:
        argument_list.append(arg.dest)
    return f'{self.prog} {" ".join(argument_list)} [ options ]{trailing_ellipsis}'

  def print_help(self, file=None):
    def bold(text):
      return ''.join( c + chr(0x08) + c for c in text)

    def underline(text, ignore_whitespace = True):
      if not ignore_whitespace:
        return ''.join('_' + chr(0x08) + c for c in text)
      return ''.join('_' + chr(0x08) + c if c != ' ' else c for c in text)

    wrapper_args = textwrap.TextWrapper(width=80, initial_indent='', subsequent_indent='                     ')
    wrapper_other = textwrap.TextWrapper(width=80, initial_indent='     ', subsequent_indent='     ')
    if self._is_project:
      text = f'Version {self._git_version}'
    else:
      text = f'MRtrix {version.VERSION}'
    text += ' ' * max(1, 40 - len(text) - int(len(self.prog)/2))
    text += bold(self.prog) + '\n'
    if self._is_project:
      text += f'using MRtrix3 {version.VERSION}\n'
    text += '\n'
    text += '     ' + bold(self.prog) + f': {"external MRtrix3 project" if self._is_project else "part of the MRtrix3 package"}\n'
    text += '\n'
    text += bold('SYNOPSIS') + '\n'
    text += '\n'
    text += wrapper_other.fill(self._synopsis) + '\n'
    text += '\n'
    text += bold('USAGE') + '\n'
    text += '\n'
    usage = self.prog + ' '
    # Compulsory subparser algorithm selection (if present)
    if self._subparsers:
      usage += f'{self._subparsers._group_actions[0].dest} [ options ] ...'
    else:
      usage += '[ options ]'
      # Find compulsory input arguments
      for arg in self._positionals._group_actions:
        usage += f' {arg.dest}'
    # Unfortunately this can line wrap early because textwrap is counting each
    #   underlined character as 3 characters when calculating when to wrap
    # Fix by underlining after the fact
    text += wrapper_other.fill(usage).replace(self.prog, underline(self.prog), 1) + '\n'
    text += '\n'
    if self._subparsers:
      text += '        ' + wrapper_args.fill(
        self._subparsers._group_actions[0].dest
        + ' '*(max(13-len(self._subparsers._group_actions[0].dest), 1))
        + self._subparsers._group_actions[0].help).replace(self._subparsers._group_actions[0].dest,
                                                           underline(self._subparsers._group_actions[0].dest), 1) \
           + '\n'
      text += '\n'
    for arg in self._positionals._group_actions:
      line = '        '
      if arg.metavar:
        name = ' '.join(arg.metavar)
      else:
        name = arg.dest
      line += f'{name}{" "*(max(13-len(name), 1))}{arg.help}'
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
        for line in wrapper_other.fill(example[0] + ':').splitlines():
          text += ' '*(len(line) - len(line.lstrip())) \
               + underline(line.lstrip(), False) \
               + '\n'
        text += f'{" "*7}$ {example[1]}\n'
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
        group_text += Parser._option2metavar(option)
        # Any options that haven't tripped one of the conditions above should be a store_true or store_false, and
        #   therefore there's nothing to be appended to the option instruction
        if isinstance(option, argparse._AppendAction):
          group_text += '  (multiple uses permitted)'
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
    text += '\n'
    text += bold('REFERENCES') + '\n'
    text += '\n'
    for entry in self._citation_list:
      if entry[0]:
        text += wrapper_other.fill('* ' + entry[0] + ':') + '\n'
      text += wrapper_other.fill(entry[1]) + '\n'
      text += '\n'
    text += wrapper_other.fill(_MRTRIX3_CORE_REFERENCE) + '\n\n'
    if file:
      file.write(text)
      file.flush()
    else:
      command = CONFIG.get('HelpCommand', 'less -X')
      if command:
        try:
          with subprocess.Popen(command.split(' '), stdin=subprocess.PIPE) as process:
            process.communicate(text.encode())
        except (subprocess.CalledProcessError, FileNotFoundError):
          sys.stdout.write(text)
          sys.stdout.flush()
      else:
        sys.stdout.write(text)
        sys.stdout.flush()

  def print_full_usage(self):
    if self._subparsers and len(sys.argv) == 3:
      for alg in self._subparsers._group_actions[0].choices:
        if alg == sys.argv[1]:
          self._subparsers._group_actions[0].choices[alg].print_full_usage()
          return
      self.error('Invalid subparser nominated')
    sys.stdout.write(f'{self._synopsis}\n')
    if self._description:
      if isinstance(self._description, list):
        for line in self._description:
          sys.stdout.write(f'{line}\n')
      else:
        sys.stdout.write(f'{self._description}\n')
    for example in self._examples:
      sys.stdout.write(f'{example[0]}: $ {example[1]}')
      if example[2]:
        sys.stdout.write(f'; {example[2]}')
      sys.stdout.write('\n')

    def arg2str(arg):
      if arg.choices:
        return f'CHOICE {" ".join(arg.choices)}'
      if isinstance(arg.type, int) or arg.type is int:
        return f'INT {-sys.maxsize - 1} {sys.maxsize}'
      if isinstance(arg.type, float) or arg.type is float:
        return 'FLOAT -inf inf'
      if isinstance(arg.type, str) or arg.type is str or arg.type is None:
        return 'TEXT'
      if isinstance(arg.type, Parser.CustomTypeBase):
        return type(arg.type)._legacytypestring()
      return arg.type._legacytypestring()

    def allow_multiple(nargs):
      return '1' if nargs in ('*', '+') else '0'

    if self._subparsers:
      sys.stdout.write(f'ARGUMENT algorithm 0 0 CHOICE {" ".join(self._subparsers._group_actions[0].choices)}\n')
    else:
      for arg in self._positionals._group_actions:
        sys.stdout.write(f'ARGUMENT {arg.dest} 0 {allow_multiple(arg.nargs)} {arg2str(arg)}\n')
        sys.stdout.write(f'{arg.help}\n')

    def print_group_options(group):
      for option in group._group_actions:
        sys.stdout.write(f'OPTION {"/".join(option.option_strings)} {"0" if option.required else "1"} {allow_multiple(option.nargs)}\n')
        sys.stdout.write(f'{option.help}\n')
        if option.nargs == 0:
          continue
        if option.metavar and isinstance(option.metavar, tuple):
          assert len(option.metavar) == option.nargs
          for arg in option.metavar:
            sys.stdout.write(f'ARGUMENT {arg} 0 0 {arg2str(option)}\n')
        else:
          multiple = allow_multiple(option.nargs)
          nargs = 1 if multiple == '1' else (option.nargs if isinstance(option.nargs, int) else 1)
          for _ in range(0, nargs):
            metavar_string = option.metavar if option.metavar else '/'.join(opt.lstrip('-') for opt in option.option_strings)
            sys.stdout.write(f'ARGUMENT {metavar_string} 0 {multiple} {arg2str(option)}\n')

    ungrouped_options = self._get_ungrouped_options()
    if ungrouped_options and ungrouped_options._group_actions:
      print_group_options(ungrouped_options)
    for group in reversed(self._action_groups):
      if self._is_option_group(group):
        print_group_options(group)
    sys.stdout.flush()

  def print_usage_markdown(self):
    if self._subparsers and len(sys.argv) == 3:
      for alg in self._subparsers._group_actions[0].choices:
        if alg == sys.argv[-2]:
          self._subparsers._group_actions[0].choices[alg].print_usage_markdown()
          return
      self.error('Invalid subparser nominated')
    text = '## Synopsis\n\n'
    text += f'{self._synopsis}\n\n'
    text += '## Usage\n\n'
    text += f'    {self.format_usage()}\n\n'
    if self._subparsers:
      text += f'-  *{self._subparsers._group_actions[0].dest}*: {self._subparsers._group_actions[0].help}\n'
    for arg in self._positionals._group_actions:
      if arg.metavar:
        name = arg.metavar
      else:
        name = arg.dest
      text += f'-  *{name}*: {arg.help}\n\n'
    if self._description:
      text += '## Description\n\n'
      for line in self._description:
        text += f'{line}\n\n'
    if self._examples:
      text += '## Example usages\n\n'
      for example in self._examples:
        text += f'__{example[0]}:__\n'
        text += f'`$ {example[1]}`\n'
        if example[2]:
          text += f'{example[2]}\n'
        text += '\n'
    text += '## Options\n\n'

    def print_group_options(group):
      group_text = ''
      for option in group._group_actions:
        option_text = '/'.join(option.option_strings)
        option_text += Parser._option2metavar(option)
        option_text = option_text.replace("<", "\\<").replace(">", "\\>")
        group_text += f'+ **-{option_text}**'
        if isinstance(option, argparse._AppendAction):
          group_text += '  *(multiple uses permitted)*'
        group_text += f'<br>{option.help}\n\n'
      return group_text

    ungrouped_options = self._get_ungrouped_options()
    if ungrouped_options and ungrouped_options._group_actions:
      text += print_group_options(ungrouped_options)
    for group in reversed(self._action_groups):
      if self._is_option_group(group):
        text += f'#### {group.title}\n\n'
        text += print_group_options(group)
    text += '## References\n\n'
    for ref in self._citation_list:
      ref_text = ''
      if ref[0]:
        ref_text += f'{ref[0]}: '
      ref_text += ref[1]
      text += f'{ref_text}\n\n'
    text += f'{_MRTRIX3_CORE_REFERENCE}\n\n'
    text += '---\n\n'
    text += f'**Author:** {self._author}\n\n'
    text += f'**Copyright:** {self._copyright}\n\n'
    sys.stdout.write(text)
    sys.stdout.flush()
    if self._subparsers:
      for alg in self._subparsers._group_actions[0].choices:
        subprocess.call ([sys.executable,
                          os.path.realpath(sys.argv[0]),
                          alg,
                          '__print_usage_markdown__'])

  def print_usage_rst(self):
    # Need to check here whether it's the documentation for a particular subparser that's being requested
    if self._subparsers and len(sys.argv) == 3:
      for alg in self._subparsers._group_actions[0].choices:
        if alg == sys.argv[-2]:
          self._subparsers._group_actions[0].choices[alg].print_usage_rst()
          return
      self.error(f'Invalid subparser nominated: {sys.argv[-2]}')
    text = f'.. _{self.prog.replace(" ", "_")}:\n\n'
    text += f'{self.prog}\n'
    text += f'{"="*len(self.prog)}\n\n'
    text += 'Synopsis\n'
    text += '--------\n\n'
    text += f'{self._synopsis}\n\n'
    text += 'Usage\n'
    text += '-----\n\n'
    text += '::\n\n'
    text += f'    {self.format_usage()}\n\n'
    if self._subparsers:
      text += f'-  *{self._subparsers._group_actions[0].dest}*: {self._subparsers._group_actions[0].help}\n'
    for arg in self._positionals._group_actions:
      if arg.metavar:
        name = arg.metavar
      else:
        name = arg.dest
      arg_help = arg.help.replace('|', '\\|')
      text += f'-  *{" ".join(name) if isinstance(name, tuple) else name}*: {arg_help}\n'
    text += '\n'
    if self._description:
      text += 'Description\n'
      text += '-----------\n\n'
      for line in self._description:
        text += f'{line}\n\n'
    if self._examples:
      text += 'Example usages\n'
      text += '--------------\n\n'
      for example in self._examples:
        text += f'-   *{example[0]}*::\n\n'
        text += f'        $ {example[1]}\n\n'
        if example[2]:
          text += f'    {example[2]}\n\n'
    text += 'Options\n'
    text += '-------\n'

    def print_group_options(group):
      group_text = ''
      for option in group._group_actions:
        option_text = '/'.join(option.option_strings)
        option_text += Parser._option2metavar(option)
        group_text += '\n'
        group_text += f'- **{option_text}**'
        if isinstance(option, argparse._AppendAction):
          group_text += '  *(multiple uses permitted)*'
        option_help = option.help.replace('|', '\\|')
        group_text += f' {option_help}\n'
      return group_text

    ungrouped_options = self._get_ungrouped_options()
    if ungrouped_options and ungrouped_options._group_actions:
      text += print_group_options(ungrouped_options)
    for group in reversed(self._action_groups):
      if self._is_option_group(group):
        text += '\n'
        text += f'{group.title}\n'
        text += f'{"^"*len(group.title)}\n'
        text += print_group_options(group)
    text += '\n'
    text += 'References\n'
    text += '^^^^^^^^^^\n\n'
    for ref in self._citation_list:
      ref_text = '* '
      if ref[0]:
        ref_text += f'{ref[0]}: '
      ref_text += ref[1]
      text += f'{ref_text}\n\n'
    text += f'{_MRTRIX3_CORE_REFERENCE}\n\n'
    text += '--------------\n\n\n\n'
    text += f'**Author:** {self._author}\n\n'
    text += f'**Copyright:** {self._copyright}\n\n'
    sys.stdout.write(text)
    sys.stdout.flush()
    if self._subparsers:
      for alg in self._subparsers._group_actions[0].choices:
        subprocess.call ([sys.executable,
                          os.path.realpath(sys.argv[0]),
                          alg,
                          '__print_usage_rst__'])

  def print_version(self):
    text = f'== {self.prog} {self._git_version if self._is_project else version.VERSION} ==\n'
    if self._is_project:
      text += f'executing against MRtrix {version.VERSION}\n'
    text += f'Author(s): {self._author}\n'
    text += f'{self._copyright}\n'
    sys.stdout.write(text)
    sys.stdout.flush()

  def _get_ungrouped_options(self):
    return next((group for group in self._action_groups if group.title in ( 'options', 'optional arguments') ), None)

  def _is_option_group(self, group):
    # * Don't display empty groups
    # * Don't display the subparser option; that's dealt with in the usage
    # * Don't re-display any compulsory positional arguments; they're also dealt with in the usage
    # * Don't display any ungrouped options; those are dealt with explicitly
    return group._group_actions and \
           not (len(group._group_actions) == 1 and \
           isinstance(group._group_actions[0], argparse._SubParsersAction)) and \
           not group == self._positionals and \
           group.title not in ( 'options', 'optional arguments' )



# Define functions for incorporating commonly-used command-line options / option groups
def add_dwgrad_import_options(cmdline): #pylint: disable=unused-variable
  options = cmdline.add_argument_group('Options for importing the diffusion gradient table')
  options.add_argument('-grad',
                       type=Parser.FileIn(),
                       metavar='file',
                       help='Provide the diffusion gradient table in MRtrix format')
  options.add_argument('-fslgrad',
                       type=Parser.FileIn(),
                       nargs=2,
                       metavar=('bvecs', 'bvals'),
                       help='Provide the diffusion gradient table in FSL bvecs/bvals format')
  cmdline.flag_mutually_exclusive_options( [ 'grad', 'fslgrad' ] )

def dwgrad_import_options(): #pylint: disable=unused-variable
  assert ARGS
  if ARGS.grad:
    return ['-grad', ARGS.grad]
  if ARGS.fslgrad:
    return ['-fslgrad', ARGS.fslgrad[0], ARGS.fslgrad[1]]
  return []




def add_dwgrad_export_options(cmdline): #pylint: disable=unused-variable
  options = cmdline.add_argument_group('Options for exporting the diffusion gradient table')
  options.add_argument('-export_grad_mrtrix',
                       type=Parser.FileOut(),
                       metavar='grad',
                       help='Export the final gradient table in MRtrix format')
  options.add_argument('-export_grad_fsl',
                       type=Parser.FileOut(),
                       nargs=2,
                       metavar=('bvecs', 'bvals'),
                       help='Export the final gradient table in FSL bvecs/bvals format')
  cmdline.flag_mutually_exclusive_options( [ 'export_grad_mrtrix', 'export_grad_fsl' ] )

def dwgrad_export_options(): #pylint: disable=unused-variable
  assert ARGS
  if ARGS.export_grad_mrtrix:
    return ['-export_grad_mrtrix', ARGS.export_grad_mrtrix]
  if ARGS.export_grad_fsl:
    return ['-export_grad_fsl', ARGS.export_grad_fsl[0], ARGS.export_grad_fsl[1]]
  return []






# Handler function for dealing with system signals
def handler(signum, _frame):
  from mrtrix3 import run #pylint: disable=import-outside-toplevel
  global SCRATCH_DIR
  # Terminate any child processes in the run module
  try:
    run.shared.terminate(signum)
  except ImportError:
    pass
  # Generate the error message
  msg = '[SYSTEM FATAL CODE: '
  signal_found = False
  for (key, value) in _SIGNALS.items():
    try:
      if getattr(signal, key) == signum:
        msg += f'{key} ({int(signum)})] {value}'
        signal_found = True
        break
    except AttributeError:
      pass
  if not signal_found:
    msg += '?] Unknown system signal'
  sys.stderr.write(f'\n{EXEC_NAME}: {ANSI.error}{msg}{ANSI.clear}\n')
  if os.getcwd() != WORKING_DIR:
    os.chdir(WORKING_DIR)
  if SCRATCH_DIR:
    if DO_CLEANUP:
      try:
        shutil.rmtree(SCRATCH_DIR)
      except OSError:
        pass
      SCRATCH_DIR = ''
    else:
      sys.stderr.write(f'{EXEC_NAME}: {ANSI.console}Scratch directory retained; location: {SCRATCH_DIR}{ANSI.clear}\n')
  for item in _STDIN_IMAGES:
    try:
      item.unlink()
    except FileNotFoundError:
      pass
  os._exit(signum) # pylint: disable=protected-access
