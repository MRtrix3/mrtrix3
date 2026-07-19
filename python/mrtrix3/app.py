# Copyright (c) 2008-2026 the MRtrix3 contributors.
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

import enum, importlib, inspect, math, os, pathlib, random, shlex, shutil, signal, string, subprocess, sys, textwrap, time
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


# This is auto-populated by script "update_copyright"
_DEFAULT_COPYRIGHT = \
'''Copyright (c) 2008-2026 the MRtrix3 contributors.

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



# Warn once for every user-specified option that the command never consulted (mirrors the C++
#   App::check_unused_options()). The tracker attached to the parsed namespace records which
#   specified options were accessed via any app.ARGS read; standard options are exempt.
def _check_unused_options():
  if ARGS is None:
    return
  tracker = vars(ARGS).get('_option_access_tracker')
  if tracker is None:
    return
  for option in tracker.unused_options():
    warn(f'Command-line option "-{option.name}" was specified but had no effect '
         f'(it may not be applicable to the operation being performed).')



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
    CMDLINE.print_synopsis()
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
  # Note that -help and -version are both handled directly within Parser.parse_args()
  #   (short-circuiting before this point), so that they operate even in the presence of
  #   otherwise-invalid command-line content; the -help check above is a redundant guard.
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
    # Read the parsed values straight from the namespace dictionary rather than via attribute
    #   access: this framework-level output-path validation must not count as the command reading
    #   the option (the unused-tracking false-positive invariant, unused-tracking-design.md 1.3).
    for value in vars(ARGS).values():
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
    # Emitted only after the command body returns normally (mirrors the C++ check_unused_options()
    #   call site, immediately after run()): a raised error supersedes this advisory warning, and
    #   -help / -version / MRTRIX_CLI_PARSE_ONLY short-circuit before reaching here.
    _check_unused_options()
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
  assert not SCRATCH_DIR, 'Cannot use multiple scratch directories'
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
#   It implements a from-scratch MRtrix3 command-line parser that mimics the C++ parser
#   (cpp/core/app.cpp), adding standard options that are common for all scripts, providing a
#   custom help page that is consistent with the MRtrix3 binaries, and defining functions for
#   exporting the help page for the purpose of automated self-documentation.

class Parser: # pylint: disable=too-many-public-methods

  # Native exception raised by the type layer (and the parser) when a command-line
  #   token cannot be interpreted. Replaces the former reliance on
  #   Parser.ArgumentError. External command modules that invoke a type callable
  #   directly (e.g. dwifslpreproc) catch app.Parser.ArgumentError.
  class ArgumentError(MRtrixError):
    pass

  # -------------------------------------------------------------------------------------
  # Native command-line data model, mirroring the C++ interface (cpp/core/cmdline_option.h).
  #   - Argument : one positional argument, OR one argument-slot within an Option;
  #                carries its own type, choices, metavar and help.
  #   - Option   : a named command-line option; its ordered list of Arguments defines its
  #                fixed arity (an empty list == a boolean flag).
  #   - OptionGroup : a named, ordered collection of Options.
  #   OptionGroup supports nesting (stage 11): a group owns an ordered list of child groups,
  #   with the invariant that a group's own direct options render before its sub-groups at
  #   every depth (see OptionGroup below).
  # -------------------------------------------------------------------------------------

  class Argument:
    # "argtype" is the raw type as supplied by the command author:
    #   None or the builtin "str" -> free text; the builtins "int"/"float"; or an
    #   instance of a Parser.CustomTypeBase subclass. Preserved verbatim so that the
    #   machine-readable exporters (stages 2-5) can reproduce the legacy type strings.
    #
    #   A tuple argument owns an ordered list of typed scalar sub-arguments ("elements"),
    #   mirroring the C++ Argument tuple model (cpp/core/cmdline_option.h): its "arity" is
    #   the number of command-line tokens it consumes (1 for a scalar, len(elements) for a
    #   tuple), and a multi-argument option is refactored to hold a single tuple argument
    #   whose elements are the former separate argument slots. Tuples do not nest.
    def __init__(self, name, *, help_text='', argtype=None, choices=None, metavar=None,
                 default=None, optional=False, allow_multiple=False, elements=None):
      self.name = name
      self.help = help_text
      self.argtype = argtype
      self.choices = choices
      self.metavar = metavar
      self.default = default
      self.optional = optional
      self.allow_multiple = allow_multiple
      self.elements = list(elements) if elements else []
      # Free-text description of the value applied when this argument / option is absent, held
      #   separately from the parse-time "default" so it may describe non-scalar defaults (e.g.
      #   "0.5 per cent", "mean"). When set it is auto-rendered as "(default: <value>)" in the
      #   help and every export, mirroring the C++ Argument::default_value (autohelp-design.md).
      self.default_value = None
      assert not any(element.is_tuple for element in self.elements), \
          'Argument tuples must not nest'

    # Declare the default value applied when this argument / option is absent, so that it is
    #   auto-rendered rather than repeated by hand in the help text. A number is formatted with
    #   the Python str() (pass a pre-formatted string to control precision or add units).
    def set_default(self, value): #pylint: disable=unused-variable
      self.default_value = value if isinstance(value, str) else str(value)
      return self

    # The auto-rendered "(choices: ...) (range: ...) (default: ...)" annotation of this argument,
    #   each clause preceded by a single space in that fixed order (empty when no such metadata);
    #   appended to the description by every human-readable help surface, mirroring the C++
    #   Argument::help_metadata() (autohelp-design.md section 2).
    def help_metadata(self):
      result = ''
      if self.choices:
        result += f' (choices: {", ".join(str(choice) for choice in self.choices)})'
      if isinstance(self.argtype, Parser.CustomTypeBase):
        result += self.argtype._help_range() # pylint: disable=protected-access
      if self.default_value is not None:
        result += f' (default: {self.default_value})'
      return result

    @property
    def is_tuple(self):
      return bool(self.elements)

    @property
    def arity(self):
      # Number of command-line tokens consumed: 1 for a scalar, len(elements) for a tuple.
      return len(self.elements) if self.elements else 1

    def leaves(self):
      # Flattened scalar sub-arguments (this argument itself when scalar); the k-th leaf
      #   corresponds to the k-th consumed token.
      return list(self.elements) if self.elements else [self]

  class Option:
    def __init__(self, name, *, help_text='', required=False, repeatable=False, dest=None,
                 default=None):
      self.name = name             # canonical spelling, WITHOUT the leading dash
      self.help = help_text
      self.required = required
      self.repeatable = repeatable  # former action='append': may be provided repeatedly
      self.dest = dest if dest is not None else name.replace('-', '_')
      self.default = default
      self.args = []               # list[Parser.Argument]; a multi-argument option holds a
                                   #   single tuple argument (its fields); [] == flag
    @property
    def is_flag(self):
      return not self.args
    @property
    def is_tuple(self):
      return any(arg.is_tuple for arg in self.args)
    @property
    def arity(self):
      # Total command-line tokens consumed: sum of the member arguments' arities.
      return sum(arg.arity for arg in self.args)
    def leaves(self):
      # Flattened scalar sub-arguments (tuples expanded); the k-th leaf corresponds to the
      #   k-th consumed token, so index-based readers address tuple fields positionally.
      result = []
      for arg in self.args:
        result.extend(arg.leaves())
      return result
    # Declare the default value applied when this option is absent, recorded on the option's sole
    #   scalar argument so that it auto-renders as "(default: <value>)" (see Argument.set_default).
    def set_default(self, value): #pylint: disable=unused-variable
      assert self.args and not self.args[0].is_tuple, \
          'set_default() is only applicable to a single-argument option'
      self.args[0].set_default(value)
      return self
    # The auto-rendered choice / range / default annotation of this option's scalar arguments,
    #   concatenated in argument order (tuple sub-arguments are excluded here; their metadata is
    #   rendered on their own listing lines), mirroring the C++ Option::help_metadata().
    def help_metadata(self):
      return ''.join(arg.help_metadata() for arg in self.args if not arg.is_tuple)

  # A named, ordered collection of Options that additionally owns an ordered list of nested
  #   child groups (sub-groups), mirroring the C++ OptionGroup (cpp/core/cmdline_option.h).
  #   A flat group has an empty "subgroups" list, so all pre-existing command usage() blocks
  #   behave identically. Nesting rule (the single ordering invariant, applied at every depth):
  #   a group's own direct options always render before its sub-groups; sub-groups render in
  #   declaration order.
  class OptionGroup:
    # The parse-time constraint a group can impose collectively on its member options
    #   (mirrors the C++ OptionGroup::Constraint, cpp/core/cmdline_option.h). A constraint is
    #   evaluated over every option in the group and, recursively, its sub-groups (i.e. over
    #   all_options()); "specified" means the option appears at least once on the command-line.
    #   The default is NONE (no constraint). Enforcement occurs during parse_args(), before any
    #   input file is accessed.
    class Constraint(enum.Enum):
      NONE = 0                 # no collective constraint (default)
      REQUIRE_EXACTLY_ONE = 1  # exactly one member option must be specified
      REQUIRE_AT_LEAST_ONE = 2 # at least one member option must be specified
      MUTUALLY_EXCLUSIVE = 3   # at most one member option may be specified
      ALL_OR_NONE = 4          # either every member option is specified, or none of them is
    def __init__(self, parser, name):
      self._parser = parser
      self.name = name
      self.options = []
      self.subgroups = []
      # The collective constraint imposed on this group's member options (see Constraint).
      self.constraint = Parser.OptionGroup.Constraint.NONE
      # True for the two standard-option groups constructed in Parser.__init__ (their nested
      #   verbosity sub-group is reached by recursion); command-defined groups are False. Used
      #   only to order constraint enforcement so that a command's own groups are checked
      #   before the standard-options groups (mirroring the C++ enforcement order).
      self.is_standard = False
    def add_argument(self, *name_or_flags, **kwargs):
      return self._parser._add_argument(self, *name_or_flags, **kwargs) # pylint: disable=protected-access
    # Nest a child group within this group, returning the new sub-group so that its own
    #   options (and any deeper sub-groups) can be registered against it.
    def add_subgroup(self, name):
      subgroup = Parser.OptionGroup(self._parser, name)
      self.subgroups.append(subgroup)
      return subgroup
    # Flattened list of all options in this group and, recursively, its sub-groups, in
    #   depth-first order: own direct options first, then each sub-group in declaration order.
    #   Every parse/match/check site iterates this so options at any depth are matchable,
    #   parseable and checkable (mirrors the C++ OptionGroup::all_options()).
    def all_options(self):
      result = list(self.options)
      for subgroup in self.subgroups:
        result.extend(subgroup.all_options())
      return result
    # Collective-constraint builder methods (mirror the C++ OptionGroup builders). Each sets
    #   this group's constraint and returns the group, so an author can parenthesise the group
    #   and apply the method inline, matching the C++ author idiom.
    def require_exactly_one(self): #pylint: disable=unused-variable
      self.constraint = Parser.OptionGroup.Constraint.REQUIRE_EXACTLY_ONE
      return self
    def require_at_least_one(self): #pylint: disable=unused-variable
      self.constraint = Parser.OptionGroup.Constraint.REQUIRE_AT_LEAST_ONE
      return self
    def mutually_exclusive(self): #pylint: disable=unused-variable
      self.constraint = Parser.OptionGroup.Constraint.MUTUALLY_EXCLUSIVE
      return self
    def all_or_none(self): #pylint: disable=unused-variable
      self.constraint = Parser.OptionGroup.Constraint.ALL_OR_NONE
      return self

  # Simple attribute container returned by parse_args(); replaces argparse.Namespace.
  #   Supports vars()/getattr()/hasattr() and in-place attribute reassignment,
  #   matching how command modules consume app.ARGS.
  #   Defining __getattr__ (a) preserves AttributeError semantics for genuinely-absent
  #   attributes so hasattr() behaves as it did with argparse.Namespace, and (b) marks
  #   the container as dynamically-populated, which suppresses spurious static-analysis
  #   no-member warnings on the many app.ARGS.<option> accesses across command modules.
  class Namespace:
    def __getattr__(self, name):
      raise AttributeError(f"'Namespace' object has no attribute '{name}'")
    # Every read of a user-facing parsed value (app.ARGS.<option>) funnels through here; this
    #   is the single choke point at which an option is marked "accessed" for the end-of-run
    #   unused-option check (mirrors the C++ ParsedOption::mark_accessed() accessor
    #   instrumentation, unused-tracking-design.md). A bare presence read counts as consulting
    #   the option, exactly as a C++ get_options() presence test does. Framework-internal state
    #   is stored under underscore-prefixed keys, which never name a command-line option, so
    #   those reads are ignored; crucially, the parser's own parse-time type coercion / validation
    #   operates on raw tokens (never through app.ARGS), so it cannot mark options accessed — this
    #   is the false-positive-avoidance invariant (unused-tracking-design.md 1.3).
    def __getattribute__(self, name):
      value = object.__getattribute__(self, name)
      if not name.startswith('_'):
        tracker = object.__getattribute__(self, '__dict__').get('_option_access_tracker')
        if tracker is not None:
          tracker.mark_accessed(name)
      return value

  # Records, for the end-of-run unused-option check, which command-line-specified options the
  #   executing command actually consulted (mirrors the C++ App::check_unused_options() state).
  #   Attached to the parsed Namespace by _parse_tokens(). An option is "specified" if it appears
  #   on the command-line, and becomes "accessed" once any app.ARGS read of its destination occurs
  #   (a bare presence test counts). Standard options are exempt: the framework consumes them
  #   uniformly and, for e.g. -nthreads, lazily, so a command that does not exercise that machinery
  #   must not be flagged for them.
  class _OptionAccessTracker:
    def __init__(self, specified_options, standard_dests):
      # specified_options: de-duplicated list of the Parser.Option objects specified on the
      #   command-line (in first-appearance order); standard_dests: the set of destination names
      #   belonging to the standard-option groups (exempt from the warning).
      self._specified = specified_options
      self._standard_dests = standard_dests
      self._accessed = set()
    def mark_accessed(self, dest):
      self._accessed.add(dest)
    def unused_options(self):
      # The specified options that were never consulted and are not standard options, each at most
      #   once (the specified list is already de-duplicated per distinct Option).
      return [option for option in self._specified
              if option.dest not in self._standard_dests and option.dest not in self._accessed]

  # Parsed value of a tuple (multi-argument) option or positional: an ordered list of the
  #   converted field values that additionally supports look-up by field id. Mirrors the
  #   C++ ParsedOption accessors (opt[k] by index / opt["field"] by name): integer indexing
  #   addresses the flattened leaves exactly as a plain list, so every pre-existing
  #   index-based reader (opt[0], opt[1], ...) keeps working unchanged, whilst the by-name
  #   accessor (opt["bvecs"]) is available where field ids are meaningful.
  class _OptionTuple(list):
    def __init__(self, values, field_names):
      super().__init__(values)
      self._field_names = list(field_names)
    def __getitem__(self, key):
      if isinstance(key, str):
        try:
          key = self._field_names.index(key)
        except ValueError as exc:
          raise KeyError(key) from exc
      return super().__getitem__(key)

  # Display id of a single leaf (scalar) argument: its metavar, falling back to its name.
  @staticmethod
  def _leaf_id(leaf):
    return leaf.metavar if leaf.metavar else leaf.name

  # The machine-readable full-usage description line for one argument: its description with the
  #   declared default value appended as free text. The default carries no dedicated machine token
  #   (adding one to the ARGUMENT line would corrupt bash-completion choice parsing); it is
  #   preserved exactly where it used to sit in the prose, mirroring the C++ Argument::usage().
  @staticmethod
  def _fullusage_desc(desc, default_value):
    if default_value is None:
      return desc
    return f'{desc} (default: {default_value})' if desc else f'(default: {default_value})'

  # Function that will create a new class,
  #   which will derive from both pathlib.Path (which itself through __new__() could be Posix or Windows)
  #   and a desired augmentation that provides additional functions
  @staticmethod
  def make_userpath_object(base_class, *args):
    abspath = os.path.normpath(os.path.join(WORKING_DIR, *args))
    super_class = pathlib.WindowsPath if os.name == 'nt' else pathlib.PosixPath
    new_class = type(f'{base_class.__name__.lstrip("_").rstrip("Extras")}',
                    (base_class, super_class),
                    {})
    if sys.version_info < (3, 12):
      instance = new_class.__new__(new_class, abspath)
    else:
      instance = new_class.__new__(new_class)
      super(super_class, instance).__init__(abspath) # pylint: disable=bad-super-call
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

  # Various callable types for use as command-line argument types
  class CustomTypeBase:
    @staticmethod
    def _legacytypestring():
      assert False
    @staticmethod
    def _metavar():
      assert False
    # The auto-rendered "(range: ...)" / "(minimum: ...)" / "(maximum: ...)" clause contributed
    #   by a bounded numeric type; empty for every unbounded / non-numeric type. Overridden by the
    #   Int / Float factories (mirrors the C++ ScalarRange rendering in Argument::help_metadata()).
    def _help_range(self):
      return ''

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
        raise Parser.ArgumentError(f'Could not interpret "{input_value}" as boolean value') from exc
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
          raise Parser.ArgumentError(f'Could not interpret "{input_value}" as integer value') from exc
        if min_value is not None and value < min_value:
          raise Parser.ArgumentError(f'Input value "{input_value}" less than minimum permissible value {min_value}')
        if max_value is not None and value > max_value:
          raise Parser.ArgumentError(f'Input value "{input_value}" greater than maximum permissible value {max_value}')
        return value
      @staticmethod
      def _legacytypestring():
        return f'INT {-sys.maxsize - 1 if min_value is None else min_value} {sys.maxsize if max_value is None else max_value}'
      @staticmethod
      def _metavar():
        return 'value'
      def _help_range(self):
        if min_value is not None and max_value is not None:
          return f' (range: {min_value} to {max_value})'
        if min_value is not None:
          return f' (minimum: {min_value})'
        if max_value is not None:
          return f' (maximum: {max_value})'
        return ''
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
          raise Parser.ArgumentError(f'Could not interpret "{input_value}" as floating-point value') from exc
        if min_value is not None and value < min_value:
          raise Parser.ArgumentError(f'Input value "{input_value}" less than minimum permissible value {min_value}')
        if max_value is not None and value > max_value:
          raise Parser.ArgumentError(f'Input value "{input_value}" greater than maximum permissible value {max_value}')
        return value
      @staticmethod
      def _legacytypestring():
        return f'FLOAT {"-inf" if min_value is None else str(min_value)} {"inf" if max_value is None else str(max_value)}'
      @staticmethod
      def _metavar():
        return 'value'
      def _help_range(self):
        # Floating-point bounds are rendered exactly as the C++ MR::str<double>() would (full
        #   double precision, "%.17g"), so that a Python range/limit clause is byte-identical to
        #   the equivalent C++ command (e.g. type_float(1e-6) -> "(minimum: 9.9999999999999995e-07)").
        if min_value is not None and max_value is not None:
          return f' (range: {min_value:.17g} to {max_value:.17g})'
        if min_value is not None:
          return f' (minimum: {min_value:.17g})'
        if max_value is not None:
          return f' (maximum: {max_value:.17g})'
        return ''
    return FloatBounded()

  # A single spherical-harmonic degree: a non-negative even integer. Mirrors the C++ scalar lmax
  #   type (cpp/core/cmdline_option.h Argument::type_lmax()): a refinement of the bounded integer
  #   type that additionally requires the value be even and non-negative. The even/sign check is
  #   applied BEFORE the magnitude-bounds check, so any parity or sign violation always reports the
  #   dedicated lmax error (including a negative value, even though the default lower bound is 0),
  #   while an in-parity, non-negative value that merely exceeds a command-specified bound reports
  #   the generic out-of-bounds error (lmax-design.md section 2.3). The default lower bound of 0
  #   encodes non-negativity through the ordinary integer range check; a command may pass a stricter
  #   lower bound (e.g. 2) or a sanity upper bound. The machine-export token is unchanged (INT).
  def Lmax(min_value=0, max_value=None): # pylint: disable=invalid-name,no-self-argument
    assert min_value is None or isinstance(min_value, int)
    assert max_value is None or isinstance(max_value, int)
    assert min_value is None or max_value is None or max_value >= min_value
    class LmaxScalar(Parser.CustomTypeBase):
      def __call__(self, input_value):
        try:
          value = int(input_value)
        except ValueError as exc:
          raise Parser.ArgumentError(f'Could not interpret "{input_value}" as integer value') from exc
        if value < 0 or value % 2:
          raise Parser.ArgumentError(f'lmax must be a non-negative even integer (value supplied: {value})')
        if min_value is not None and value < min_value:
          raise Parser.ArgumentError(f'Input value "{input_value}" less than minimum permissible value {min_value}')
        if max_value is not None and value > max_value:
          raise Parser.ArgumentError(f'Input value "{input_value}" greater than maximum permissible value {max_value}')
        return value
      @staticmethod
      def _legacytypestring():
        return f'INT {-sys.maxsize - 1 if min_value is None else min_value} {sys.maxsize if max_value is None else max_value}'
      @staticmethod
      def _metavar():
        return 'value'
      def _help_range(self):
        if min_value is not None and max_value is not None:
          base = f' (range: {min_value} to {max_value})'
        elif min_value is not None:
          base = f' (minimum: {min_value})'
        elif max_value is not None:
          base = f' (maximum: {max_value})'
        else:
          base = ''
        return f'{base} (must be even)'
    return LmaxScalar()

  class SequenceInt(CustomTypeBase):
    def __call__(self, input_value):
      try:
        return [int(i) for i in input_value.split(',')]
      except ValueError as exc:
        raise Parser.ArgumentError(f'Could not interpret "{input_value}" as integer sequence') from exc
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
        raise Parser.ArgumentError(f'Could not interpret "{input_value}" as floating-point sequence') from exc
    @staticmethod
    def _legacytypestring():
      return 'FSEQ'
    @staticmethod
    def _metavar():
      return 'values'

  # A comma-separated list of spherical-harmonic degrees, each a non-negative even integer.
  #   Mirrors the C++ vector lmax type (Argument::type_lmax_sequence()): a refinement of the
  #   integer-sequence type that validates each parsed element. The per-element error names the
  #   owning option/argument, so the parser records that source on the instance when the option
  #   is declared (see _add_argument). Negative entries are unified onto the same lmax message
  #   (lmax-design.md section 2.4). The machine-export token is unchanged (ISEQ).
  class SequenceLmax(CustomTypeBase):
    def __init__(self):
      self._source = None
    def __call__(self, input_value):
      try:
        values = [int(i) for i in input_value.split(',')]
      except ValueError as exc:
        raise Parser.ArgumentError(f'Could not interpret "{input_value}" as integer sequence') from exc
      for value in values:
        if value < 0 or value % 2:
          source = self._source if self._source is not None else 'the lmax sequence'
          raise Parser.ArgumentError(f'each lmax value supplied for {source} '
                                     f'must be a non-negative even integer (value supplied: {value})')
      return values
    @staticmethod
    def _legacytypestring():
      return 'ISEQ'
    @staticmethod
    def _metavar():
      return 'values'
    def _help_range(self):
      return ' (values must be non-negative and even)'

  class DirectoryIn(CustomTypeBase):
    def __call__(self, input_value):
      abspath = Parser.make_userpath_object(Parser._UserPathExtras, input_value)
      if not abspath.exists():
        raise Parser.ArgumentError(f'Input directory "{input_value}" does not exist')
      if not abspath.is_dir():
        raise Parser.ArgumentError(f'Input path "{input_value}" is not a directory')
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
        raise Parser.ArgumentError(f'Input file "{input_value}" does not exist')
      if not abspath.is_file():
        raise Parser.ArgumentError(f'Input path "{input_value}" is not a file')
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
        raise Parser.ArgumentError(f'Input tractogram file "{filepath}" is not a valid track file')
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
        raise Parser.ArgumentError(f'Output tractogram path "{filepath}" does not use the requisite ".tck" suffix')
      return filepath
    @staticmethod
    def _legacytypestring():
      return 'TRACKSOUT'
    @staticmethod
    def _metavar():
      return 'trackfile'





  def __init__(self, parents=None):
    self._author = None
    self._citation_list = [ ]
    self._copyright = _DEFAULT_COPYRIGHT
    self._description = [ ]
    self._examples = [ ]
    self._external_citations = False
    self._synopsis = None
    self.prog = EXEC_NAME
    self._positional_args = [ ]
    # Ordered option groups. Index 0 is the "ungrouped" group, whose options are rendered
    #   first and without a group heading (matching the former argparse default options
    #   group). Standard-option groups follow; command-specific groups are appended by
    #   add_argument_group().
    self._ungrouped = Parser.OptionGroup(self, 'OPTIONS')
    self._option_groups = [ self._ungrouped ]
    # Command-declared cross-group mutual-exclusion sets (the analogue of the C++
    #   MUTUALLY_EXCLUSIVE_OPTIONS global): a list of lists of canonical option ids, each list
    #   meaning "at most one of these may be specified". Populated by
    #   flag_mutually_exclusive_options(); enforced at parse time.
    self._mutually_exclusive_option_groups = [ ]
    self._help_option = None
    self._version_option = None
    # Populated by add_subparsers() on a multi-algorithm command's top-level parser;
    #   remains None both for single-level commands and for the base / per-algorithm
    #   parsers constructed underneath a subparser command.
    self._subparsers = None
    # A parser constructed with parents (the per-algorithm sub-parsers and the shared base
    #   parser) inherits the standard-option groups, ungrouped options, help/version option
    #   handles and citation list of its parent(s) rather than creating its own; this
    #   reinstates the citation- and option-inheritance that the argparse "parents="
    #   mechanism formerly provided (see stage 6 of the CLI overhaul).
    if parents is not None:
      for parent in parents:
        self._citation_list.extend(parent._citation_list)
        self._external_citations = self._external_citations or parent._external_citations
        self._ungrouped.options.extend(parent._ungrouped.options)
        for group in parent._option_groups[1:]:
          self._option_groups.append(group)
        # Inherit any cross-group mutual-exclusion sets declared on the parent (e.g. those a
        #   multi-algorithm command registers on its top-level parser before add_subparsers()),
        #   so that each per-algorithm sub-parser enforces them too.
        self._mutually_exclusive_option_groups.extend(parent._mutually_exclusive_option_groups)
        if parent._help_option is not None:
          self._help_option = parent._help_option
        if parent._version_option is not None:
          self._version_option = parent._version_option
      # The project / version metadata is identical to that of the parent(s); copy it
      #   rather than re-invoking "git describe" once per sub-parser.
      self._is_project = parents[0]._is_project
      self._git_version = parents[0]._git_version
      return
    standard_options = self.add_argument_group('Standard options')
    standard_options.is_standard = True
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
    self._help_option = standard_options.add_argument('-help',
                                  action='store_true',
                                  default=None,
                                  help='display this information page and exit.')
    self._version_option = standard_options.add_argument('-version',
                                  action='store_true',
                                  default=None,
                                  help='display version information and exit.')
    # The verbosity trio ( -info / -quiet / -debug ) is nested as a "Verbosity options"
    #   sub-group of Standard options (mirroring the C++ parser); per the ordering invariant
    #   it therefore renders after the remaining standard options above. The former ad-hoc
    #   mutual-exclusion of the trio is reinstated here as a principled group constraint
    #   (stages 12/13): the three levels are alternatives, so at most one may be specified;
    #   the default (no flag) is normal verbosity and remains valid, so no command requires a
    #   verbosity flag.
    verbosity_options = standard_options.add_subgroup('Verbosity options')
    verbosity_options.add_argument('-info',
                                  action='store_true',
                                  default=None,
                                  help='display information messages.')
    verbosity_options.add_argument('-quiet',
                                  action='store_true',
                                  default=None,
                                  help='do not display information messages or progress status. '
                                       'Alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.')
    verbosity_options.add_argument('-debug',
                                  action='store_true',
                                  default=None,
                                  help='display debugging messages & debug input data.')
    verbosity_options.mutually_exclusive()
    script_options = self.add_argument_group('Additional standard options for Python scripts')
    script_options.is_standard = True
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

  def flag_mutually_exclusive_options(self, ids): #pylint: disable=unused-variable
    # Declare a cross-group mutual-exclusion set: at most one of the named options may be
    #   specified on the command-line, regardless of which option group each belongs to
    #   (mirrors the C++ MUTUALLY_EXCLUSIVE_OPTIONS, cpp/core/app.h). Options are named by
    #   canonical id (without the leading dash) and resolved across the whole option
    #   hierarchy. Where the conflicting options constitute an entire option group, prefer
    #   the group-level OptionGroup.mutually_exclusive() instead.
    assert isinstance(ids, (list, tuple)) and all(isinstance(item, str) for item in ids), \
        'Parser.flag_mutually_exclusive_options() accepts a list of option-name strings'
    self._mutually_exclusive_option_groups.append(list(ids))

  def add_argument_group(self, name): #pylint: disable=unused-variable
    group = Parser.OptionGroup(self, name)
    self._option_groups.append(group)
    return group

  def add_argument(self, *name_or_flags, **kwargs): #pylint: disable=unused-variable
    # Options added directly on the parser (rather than on a group returned by
    #   add_argument_group()) land in the ungrouped 'OPTIONS' group; positional
    #   arguments are always collected on the parser regardless of the group used.
    return self._add_argument(self._ungrouped, *name_or_flags, **kwargs)

  # Unified constructor for both positional Arguments and Options, preserving the
  #   command-author keyword protocol that argparse previously supplied. Recognised
  #   keywords: help, type, nargs, metavar, action, default, choices, dest, allow_multiple.
  def _add_argument(self, group, *name_or_flags, **kwargs):
    assert len(name_or_flags) == 1, \
        'MRtrix3 add_argument() accepts exactly one argument/option name'
    name = name_or_flags[0]
    help_text = kwargs.pop('help', None)
    argtype = kwargs.pop('type', None)
    nargs = kwargs.pop('nargs', None)
    metavar = kwargs.pop('metavar', None)
    action = kwargs.pop('action', None)
    default = kwargs.pop('default', None)
    choices = kwargs.pop('choices', None)
    dest = kwargs.pop('dest', None)
    allow_multiple = kwargs.pop('allow_multiple', False)
    assert not kwargs, f'Unsupported keyword arguments to add_argument({name}): {kwargs}'
    assert action in (None, 'store_true', 'append'), \
        f'Unsupported action "{action}" for add_argument({name})'
    if choices is not None:
      choices = list(choices)

    if name.startswith('-'):
      # ---- Command-line option ----
      opt_name = name.lstrip('-')
      # The vector lmax type embeds the owning option in its per-element error; record it here,
      #   mirroring the C++ parse-time source (option id) passed to check_lmax_sequence().
      if isinstance(argtype, Parser.SequenceLmax):
        argtype._source = f'option "-{opt_name}"' # pylint: disable=protected-access
      option = Parser.Option(opt_name,
                             help_text=help_text,
                             repeatable=action == 'append',
                             dest=dest,
                             default=default)
      if action == 'store_true':
        assert nargs in (None, 0), f'store_true option {name} cannot take arguments'
      else:
        assert nargs not in ('+', '*', '?'), \
            (f'Arbitrary-arity options are not supported (option {name}, nargs={nargs!r}); '
             'declare a fixed number of arguments, each with its own type and help')
        arity = nargs if isinstance(nargs, int) else 1
        metavar_tuple = metavar if isinstance(metavar, tuple) else None
        def make_slot(slot_index):
          if metavar_tuple is not None:
            slot_metavar = metavar_tuple[slot_index]
          elif isinstance(metavar, str):
            slot_metavar = metavar
          else:
            slot_metavar = None
          # A tuple element carries no per-field help (the Python author API supplies a
          #   single option-level help string); the field-description rendering therefore
          #   emits nothing for these options, keeping their exports byte-identical.
          return Parser.Argument(opt_name,
                                 help_text='' if arity > 1 else help_text,
                                 argtype=argtype,
                                 choices=choices,
                                 metavar=slot_metavar)
        if arity > 1:
          # Multi-argument option: a single tuple argument whose elements are the fields.
          option.args.append(Parser.Argument(opt_name,
                                              help_text=help_text,
                                              argtype=argtype,
                                              choices=choices,
                                              elements=[make_slot(i) for i in range(arity)]))
        else:
          option.args.append(make_slot(0))
      group.options.append(option)
      return option

    # ---- Positional argument ----
    # Variable-count positionals ( former nargs='+'/'*' ) are expressed natively via
    #   allow_multiple, mirroring the C++ Argument::allow_multiple(); at most one
    #   positional per command may be variable-count.
    if nargs in ('+', '*'):
      allow_multiple = True
    if isinstance(argtype, Parser.SequenceLmax):
      argtype._source = f'argument "{dest if dest is not None else name}"' # pylint: disable=protected-access
    argument = Parser.Argument(dest if dest is not None else name,
                               help_text=help_text,
                               argtype=argtype,
                               choices=choices,
                               metavar=metavar if isinstance(metavar, str) else None,
                               default=default,
                               optional=nargs in ('?', '*'),
                               allow_multiple=allow_multiple)
    self._positional_args.append(argument)
    return argument

  # Registry of a multi-algorithm command's per-algorithm sub-parsers.
  #   Replaces the former argparse "_SubParsersAction". Each algorithm module's
  #   usage(base_parser, subparsers) function calls add_parser() to register its
  #   sub-interface; the resulting Parser inherits the base parser's options and
  #   citations via the "parents=" mechanism (see Parser.__init__).
  class _SubParsers:
    def __init__(self, base_parser, algorithms):
      self._base_parser = base_parser
      self.dest = 'algorithm'
      self.algorithms = list(algorithms)
      self.choices = { }  # ordered name -> Parser, in ALGORITHMS order
      self.help = ('Select the algorithm to be used; '
                   'additional details and options become available once an algorithm is nominated. '
                   'Options are: ' + ', '.join(self.algorithms))
    def add_parser(self, name, parents=None): # pylint: disable=unused-variable
      child = Parser(parents=parents if parents is not None else [self._base_parser])
      child.prog = f'{EXEC_NAME} {name}'
      self.choices[name] = child
      return child

  # Reintroduce algorithm-dispatch sub-parsers (stage 6 of the CLI overhaul). Invoked from
  #   the "usage" function of a multi-algorithm command, after that command has registered
  #   any options common to all its algorithms. Builds a base parser carrying the standard
  #   options plus those common options, then lets each algorithm register its own
  #   sub-interface against that base via its usage(base_parser, subparsers) function.
  def add_subparsers(self): # pylint: disable=unused-variable
    module_name = os.path.dirname(inspect.getouterframes(inspect.currentframe())[1].filename).split(os.sep)[-1]
    module = sys.modules[f'mrtrix3.commands.{module_name}']
    # The base parser inherits, via the parents= mechanism, the standard options, this
    #   command's common options (registered on "self" before add_subparsers() was called)
    #   and the top-level citations; each per-algorithm parser in turn inherits from it.
    base_parser = Parser(parents=[self])
    self._subparsers = Parser._SubParsers(base_parser, module.ALGORITHMS)
    for algorithm in module.ALGORITHMS:
      algorithm_module = importlib.import_module(f'.{algorithm}', f'mrtrix3.commands.{module_name}')
      algorithm_module.usage(base_parser, self._subparsers)

  def print_citation_warning(self):
    # If a subparser was invoked, defer to the chosen algorithm's parser, since it may
    #   carry additional (possibly external) citations beyond those of the top-level command.
    if self._subparsers is not None:
      chosen = getattr(ARGS, self._subparsers.dest, None)
      if chosen is not None and chosen in self._subparsers.choices:
        self._subparsers.choices[chosen].print_citation_warning()
        return
    if not self._external_citations:
      return
    console('')
    console('Note that this script may make use of commands / algorithms'
            ' from neuroimaging software other than MRtrix3.')
    console('PLEASE ENSURE that any non-MRtrix3 software,'
            ' as well as any research methods they provide,'
            ' are recognised and cited appropriately,'
            ' and that any relevant software usage licenses are not violated.')
    console('Consult the help page (-help option) for more information.')
    console('')

  # -------------------------------------------------------------------------------------
  # Command-line parsing, mimicking the C++ parser (cpp/core/app.cpp):
  #   single-dash whole-word options, bulk leading-dash stripping, unambiguous-prefix
  #   matching (exact beats ambiguous), fixed-arity consumption, permutability of options
  #   amongst positionals, and '-'/negative-number passthrough.
  # -------------------------------------------------------------------------------------

  def _iter_options(self):
    # Recurses into nested sub-groups (all_options()) so that options at any depth
    #   (e.g. the nested verbosity trio) are matchable, parseable and checkable.
    for group in self._option_groups:
      yield from group.all_options()

  @staticmethod
  def _without_leading_dashes(token):
    index = 0
    while index < len(token) and token[index] == '-':
      index += 1
    return token[index:]

  # Returns the matched Option, or None when the token is not option-like (no leading
  #   dash, a bare '-', or a value beginning with a digit or '.', all of which pass
  #   through as positional content). Raises ArgumentError for an unknown or ambiguous
  #   option, using the same wording as the C++ parser.
  def _match_option(self, token):
    root = self._without_leading_dashes(token)
    if len(root) == len(token) or not root or root[0].isdigit() or root[0] == '.':
      return None
    candidates = [option for option in self._iter_options() if option.name.startswith(root)]
    if not candidates:
      raise Parser.ArgumentError(f'unknown option "-{root}"')
    if len(candidates) == 1:
      return candidates[0]
    for candidate in candidates:
      if candidate.name == root:
        return candidate
    first_name = candidates[0].name
    if all(candidate.name == first_name for candidate in candidates):
      return candidates[0]
    quoted = '", "-'.join(candidate.name for candidate in candidates)
    raise Parser.ArgumentError(f'several matches possible for option "-{root}": "-{quoted}"')

  @staticmethod
  def _convert_value(spec, value, context):
    if spec.choices is not None and value not in spec.choices:
      raise Parser.ArgumentError(f'{context}: unexpected value "{value}"; '
                                 f'expected one of: {", ".join(spec.choices)}')
    argtype = spec.argtype
    if argtype is None or argtype is str:
      return value
    # A Parser.ArgumentError raised by the type callable propagates unchanged (it is not a
    #   ValueError/TypeError); builtin int()/float() failures are wrapped for context.
    try:
      return argtype(value)
    except (ValueError, TypeError) as exc:
      raise Parser.ArgumentError(f'{context}: {exc}') from exc

  def _error(self, message):
    sys.stderr.write('\n')
    sys.stderr.write(f'{self.prog}: {ANSI.error}[ERROR] {message}{ANSI.clear}\n')
    sys.stderr.write(f'{self.prog}: {ANSI.console}Usage: {self.format_usage()}{ANSI.clear}\n')
    sys.stderr.write(f'{self.prog}: {ANSI.console}       '
                     f'(Run {self.prog} -help for more information){ANSI.clear}\n')
    sys.stderr.flush()
    sys.exit(1)

  # -help / -version take precedence and short-circuit, so that they operate even in the
  #   presence of otherwise-invalid command-line content (matching the C++ parser). The
  #   options are matched against, and the resulting page rendered from, "target" — which
  #   is the top-level parser for a single-level command, or the selected algorithm's
  #   sub-parser once an algorithm has been identified (giving per-algorithm -help).
  @staticmethod
  def _prescan_help_version(tokens, target):
    for token in tokens:
      try:
        option = target._match_option(token) # pylint: disable=protected-access
      except Parser.ArgumentError:
        option = None
      if option is not None and option is target._help_option: # pylint: disable=protected-access
        target.print_help()
        sys.exit(0)
      if option is not None and option is target._version_option: # pylint: disable=protected-access
        target.print_version()
        sys.exit(0)

  def parse_args(self):
    assert self._author, 'Script author MUST be set in script\'s usage() function'
    assert self._synopsis, 'Script synopsis MUST be set in script\'s usage() function'
    tokens = sys.argv[1:]
    if self._subparsers is not None:
      return self._parse_subparser_args(tokens)
    Parser._prescan_help_version(tokens, self)
    return self._parse_tokens(tokens)

  # Locate the algorithm token: the first token that is not an option nor an argument to a
  #   preceding option. Options preceding the algorithm are recognised (and their arguments
  #   skipped) via the top-level parser's option set (the standard + common options), which
  #   is exactly the set permitted before the algorithm; this is what makes those options
  #   permutable with, and reachable by, the selected algorithm. Returns (name, index), or
  #   (None, None) if no algorithm token is present.
  def _find_algorithm(self, tokens):
    index = 0
    while index < len(tokens):
      option = self._match_option(tokens[index])
      if option is None:
        return tokens[index], index
      index += 1 if option.is_flag else 1 + option.arity
    return None, None

  def _parse_subparser_args(self, tokens):
    algorithm, algorithm_index = None, None
    try:
      algorithm, algorithm_index = self._find_algorithm(tokens)
    except Parser.ArgumentError as exception:
      self._error(str(exception))
    # Route -help / -version to the selected algorithm's sub-parser when one has been
    #   identified, else to the top-level command.
    if algorithm is not None and algorithm in self._subparsers.choices:
      Parser._prescan_help_version(tokens, self._subparsers.choices[algorithm])
    else:
      Parser._prescan_help_version(tokens, self)
    if algorithm is None:
      self._error(f'no algorithm selected (expected one of: {", ".join(self._subparsers.algorithms)})')
    if algorithm not in self._subparsers.choices:
      self._error(f'unknown algorithm "{algorithm}" (expected one of: {", ".join(self._subparsers.algorithms)})')
    child = self._subparsers.choices[algorithm]
    # All tokens except the algorithm name itself are parsed by the algorithm's sub-parser,
    #   so options given either side of the algorithm name reach the algorithm's execute().
    algorithm_tokens = tokens[:algorithm_index] + tokens[algorithm_index + 1:]
    namespace = child._parse_tokens(algorithm_tokens) # pylint: disable=protected-access
    setattr(namespace, self._subparsers.dest, algorithm)
    return namespace

  def _parse_tokens(self, tokens):
    namespace = Parser.Namespace()
    for option in self._iter_options():
      setattr(namespace, option.dest, option.default)
    for argument in self._positional_args:
      setattr(namespace, argument.name, [] if argument.allow_multiple else argument.default)
    positional_tokens = [ ]
    # The options actually specified on the command-line, in order of first appearance (the
    #   analogue of the C++ "option" vector), used to evaluate the collective group / cross-
    #   group constraints. An option is recorded once regardless of repetition.
    specified = [ ]
    try:
      index = 0
      while index < len(tokens):
        token = tokens[index]
        option = self._match_option(token)
        if option is None:
          positional_tokens.append(token)
          index += 1
          continue
        if option not in specified:
          specified.append(option)
        if option.is_flag:
          setattr(namespace, option.dest, True)
          index += 1
          continue
        arity = option.arity
        if index + arity >= len(tokens):
          raise Parser.ArgumentError(f'not enough parameters to option "-{option.name}"')
        raw = tokens[index + 1 : index + 1 + arity]
        context = f'error parsing argument to option "-{option.name}"'
        converted = [self._convert_value(leaf, value, context)
                     for leaf, value in zip(option.leaves(), raw)]
        # A tuple option's parsed value is an _OptionTuple (index- and name-addressable);
        #   a scalar single-argument option yields its lone converted value. Repeatable
        #   options accumulate one entry per use (the historical list form is preserved
        #   for scalar repeatable options such as for_each's -exclude).
        if option.is_tuple:
          value = Parser._OptionTuple(converted, [Parser._leaf_id(leaf) for leaf in option.leaves()])
        else:
          value = converted if option.repeatable else converted[0]
        if option.repeatable:
          existing = getattr(namespace, option.dest)
          if existing is None:
            existing = [ ]
            setattr(namespace, option.dest, existing)
          existing.append(value)
        else:
          setattr(namespace, option.dest, value)
        index += 1 + arity
      self._assign_positionals(namespace, positional_tokens)
      for option in self._iter_options():
        if option.required and getattr(namespace, option.dest) is None:
          raise Parser.ArgumentError(f'mandatory option "-{option.name}" was not provided')
      # Enforce the collective option-group constraints and cross-group mutual-exclusion sets,
      #   immediately after the per-option required check and before any input file is opened,
      #   matching the C++ enforcement point (cpp/core/app.cpp parse()).
      self._enforce_constraints(specified)
    except Parser.ArgumentError as exception:
      self._error(str(exception))
    # Attach the per-option access tracker used by the end-of-run unused-option check. Only the
    #   successful parse reaches here (a parse error exits via _error()). Standard-option
    #   destinations (the two standard-option groups and, recursively, the nested verbosity
    #   sub-group) are exempt from the warning.
    standard_dests = set()
    for group in self._option_groups:
      if group.is_standard:
        for option in group.all_options():
          standard_dests.add(option.dest)
    namespace.__dict__['_option_access_tracker'] = Parser._OptionAccessTracker(specified, standard_dests)
    return namespace

  # ---- Collective option-group constraint enforcement (mirrors cpp/core/app.cpp) ------------
  # "specified" is the list of Option objects present on the command-line (see _parse_tokens).

  # The subset of the given options that were specified, as "-id" strings, in the options'
  #   own order, each at most once (mirrors the C++ specified_options()).
  @staticmethod
  def _specified_option_ids(options, specified):
    return [f'-{option.name}' for option in options if option in specified]

  # Comma-joined "-id" list of every given option, whether specified or not (C++ all_option_ids()).
  @staticmethod
  def _all_option_ids(options):
    return ', '.join(f'-{option.name}' for option in options)

  # Evaluate one group's collective constraint over its (recursive) member options, raising an
  #   ArgumentError with the C++ wording (constraints-design.md section 4) on a violation.
  def _enforce_group_constraint(self, group, specified):
    enum_cls = Parser.OptionGroup.Constraint
    if group.constraint == enum_cls.NONE:
      return
    members = group.all_options()
    present = self._specified_option_ids(members, specified)
    if group.constraint == enum_cls.REQUIRE_EXACTLY_ONE:
      if not present:
        raise Parser.ArgumentError('exactly one of the following options must be specified: '
                                   f'{self._all_option_ids(members)}')
      if len(present) > 1:
        raise Parser.ArgumentError(f'the options {", ".join(present)} are mutually exclusive; '
                                   'exactly one must be specified')
    elif group.constraint == enum_cls.REQUIRE_AT_LEAST_ONE:
      if not present:
        raise Parser.ArgumentError('at least one of the following options must be specified: '
                                   f'{self._all_option_ids(members)}')
    elif group.constraint == enum_cls.MUTUALLY_EXCLUSIVE:
      if len(present) > 1:
        raise Parser.ArgumentError(f'the options {", ".join(present)} are mutually exclusive; '
                                   'at most one may be specified')
    elif group.constraint == enum_cls.ALL_OR_NONE:
      if present and len(present) != len(members):
        raise Parser.ArgumentError(f'the options {self._all_option_ids(members)} must be specified '
                                   f'together or not at all; only {", ".join(present)} specified')

  # Recursively enforce a group's constraint and those of all its nested sub-groups.
  def _enforce_group_constraints(self, group, specified):
    self._enforce_group_constraint(group, specified)
    for subgroup in group.subgroups:
      self._enforce_group_constraints(subgroup, specified)

  # Enforce the command-declared cross-group mutual-exclusion sets (C++ enforce_cross_group_mutex()).
  def _enforce_cross_group_mutex(self, specified):
    specified_names = [option.name for option in specified]
    for id_set in self._mutually_exclusive_option_groups:
      present = [f'-{item}' for item in id_set if item in specified_names]
      if len(present) > 1:
        raise Parser.ArgumentError(f'the options {", ".join(present)} are mutually exclusive; '
                                   'at most one may be specified')

  # Enforce all collective constraints in the C++ evaluation order: the command's own option
  #   groups (declaration order, recursing sub-groups depth-first), then the standard-options
  #   groups (so the nested verbosity sub-group is checked), then the cross-group mutex sets.
  #   The first violation raises.
  def _enforce_constraints(self, specified):
    for group in self._option_groups:
      if not group.is_standard:
        self._enforce_group_constraints(group, specified)
    for group in self._option_groups:
      if group.is_standard:
        self._enforce_group_constraints(group, specified)
    self._enforce_cross_group_mutex(specified)

  def _assign_positionals(self, namespace, values):
    # Counting is done in tokens: each positional spec consumes "arity" tokens (a tuple
    #   consumes one token per field), reducing to the historical argument-count algorithm
    #   when every arity is 1. Mirrors the C++ arity-aware positional handling
    #   (cpp/core/app.cpp): a repeatable spec absorbs surplus tokens in whole groups of its
    #   arity, and an indivisible surplus is an error.
    specs = self._positional_args
    count = len(values)

    # Bind one positional spec to its list of raw tokens, converting each leaf. A tuple
    #   spec yields one _OptionTuple per group of "arity" tokens (fields bound cyclically);
    #   a scalar spec yields a single value, or a flat list when repeatable.
    def assign(spec, tokens):
      context = f'error parsing argument "{spec.name}"'
      if spec.is_tuple:
        leaves = spec.leaves()
        names = [Parser._leaf_id(leaf) for leaf in leaves]
        groups = [Parser._OptionTuple([self._convert_value(leaves[offset], tokens[base + offset], context)
                                       for offset in range(spec.arity)],
                                      names)
                  for base in range(0, len(tokens), spec.arity)]
        if spec.allow_multiple:
          setattr(namespace, spec.name, groups)
        elif groups:
          setattr(namespace, spec.name, groups[0])
      else:
        converted = [self._convert_value(spec, token, context) for token in tokens]
        if spec.allow_multiple:
          setattr(namespace, spec.name, converted)
        elif converted:
          setattr(namespace, spec.name, converted[0])

    multi_index = next((i for i, spec in enumerate(specs) if spec.allow_multiple), None)
    if multi_index is None:
      required_tokens = sum(spec.arity for spec in specs if not spec.optional)
      total_tokens = sum(spec.arity for spec in specs)
      if count < required_tokens:
        raise Parser.ArgumentError(f'expected {required_tokens} positional argument'
                                   f'{"s" if required_tokens != 1 else ""}, received {count}')
      if count > total_tokens:
        raise Parser.ArgumentError(f'expected at most {total_tokens} positional argument'
                                   f'{"s" if total_tokens != 1 else ""}, received {count}')
      index = 0
      for spec in specs:
        if index + spec.arity <= count:
          assign(spec, values[index : index + spec.arity])
          index += spec.arity
        else:
          # A trailing optional spec with no remaining tokens keeps its default.
          break
    else:
      before = specs[:multi_index]
      after = specs[multi_index + 1:]
      multi = specs[multi_index]
      before_tokens = sum(spec.arity for spec in before)
      after_tokens = sum(spec.arity for spec in after)
      minimum = before_tokens + after_tokens + (0 if multi.optional else multi.arity)
      if count < minimum:
        raise Parser.ArgumentError(f'not enough positional arguments '
                                   f'(expected at least {minimum}, received {count})')
      multi_token_count = count - before_tokens - after_tokens
      if multi_token_count % multi.arity != 0:
        raise Parser.ArgumentError('number of optional arguments provided '
                                   'are not equal for all arguments')
      index = 0
      for spec in before:
        assign(spec, values[index : index + spec.arity])
        index += spec.arity
      assign(multi, values[index : index + multi_token_count])
      index += multi_token_count
      for spec in after:
        assign(spec, values[index : index + spec.arity])
        index += spec.arity

  def format_usage(self):
    argument_list = [ ]
    trailing_ellipsis = ''
    # A multi-algorithm command presents the algorithm as its (only) leading positional,
    #   with a trailing ellipsis standing in for the algorithm-specific arguments/options.
    if self._subparsers is not None:
      argument_list.append(self._subparsers.dest)
      trailing_ellipsis = ' ...'
    for argument in self._positional_args:
      if argument.metavar:
        argument_list.append(argument.metavar)
      else:
        argument_list.append(argument.name)
    return f'{self.prog} {" ".join(argument_list)} [ options ]{trailing_ellipsis}'

  # Metavar string for an option, rendered for the terminal help page.
  #   Returns '' for a boolean flag, otherwise a leading-space-prefixed, space-separated
  #   list of per-argument metavars.
  @staticmethod
  def _option_metavar(option):
    if option.is_flag:
      return ''
    parts = [ ]
    for slot in option.leaves():
      if slot.metavar is not None:
        parts.append(slot.metavar)
      elif slot.choices is not None:
        parts.append('choice')
      elif isinstance(slot.argtype, Parser.CustomTypeBase):
        parts.append(slot.argtype._metavar()) # pylint: disable=protected-access
      elif slot.argtype in (int, float):
        parts.append(slot.argtype.__name__.lower())
      elif slot.argtype is str:
        parts.append('str')
      else:
        parts.append('string')
    return ' ' + ' '.join(parts)

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
    # A multi-algorithm command presents the compulsory algorithm selection in place of
    #   fixed positional arguments; the algorithm-specific arguments/options are then
    #   summarised by the trailing ellipsis.
    if self._subparsers is not None:
      usage += f'{self._subparsers.dest} [ options ] ...'
    else:
      usage += '[ options ]'
      # Find compulsory input arguments
      for argument in self._positional_args:
        usage += f' {argument.name}'
    # Unfortunately this can line wrap early because textwrap is counting each
    #   underlined character as 3 characters when calculating when to wrap
    # Fix by underlining after the fact
    text += wrapper_other.fill(usage).replace(self.prog, underline(self.prog), 1) + '\n'
    text += '\n'
    if self._subparsers is not None:
      dest = self._subparsers.dest
      text += '        ' + wrapper_args.fill(
        dest + ' '*(max(13-len(dest), 1)) + self._subparsers.help).replace(dest, underline(dest), 1) + '\n'
      text += '\n'
    for argument in self._positional_args:
      line = '        '
      name = argument.metavar if argument.metavar else argument.name
      line += f'{name}{" "*(max(13-len(name), 1))}{argument.help}{argument.help_metadata()}'
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

    # Define a function for printing all text for a given option group.
    # This is used in two separate locations:
    #   - First printing any ungrouped command-line options;
    #   - Printing all contents of each named option group.
    wrapper_field = textwrap.TextWrapper(width=80, initial_indent='       ', subsequent_indent='       ')
    def print_group_options(group):
      group_text = ''
      for option in group.options:
        group_text += '  ' + underline('-' + option.name)
        group_text += Parser._option_metavar(option)
        if option.repeatable:
          group_text += '  (multiple uses permitted)'
        group_text += '\n'
        group_text += wrapper_other.fill(option.help + option.help_metadata()) + '\n'
        # A described tuple field is listed beneath the option (indented past the option
        #   help); scalar options add nothing here. A field's own choice / range / default
        #   metadata is appended to its description (rendered even when it has no description).
        for arg in option.args:
          if not arg.is_tuple:
            continue
          for leaf in arg.elements:
            leaf_desc = leaf.help + leaf.help_metadata()
            if leaf_desc:
              leaf_id = Parser._leaf_id(leaf)
              group_text += wrapper_field.fill(f'{leaf_id}: {leaf_desc}') + '\n'
        group_text += '\n'
      return group_text

    # Render a group's nested child groups after its own options: each sub-group's bold
    #   header is indented by two spaces per level of depth (the depth cue; the nested
    #   options themselves are not further indented), then its options, then recursively
    #   its own sub-groups. "depth" is the parent group's depth.
    def print_subgroups(group, depth):
      subgroup_text = ''
      for subgroup in group.subgroups:
        subgroup_text += '  ' * (depth + 1) + bold(subgroup.name) + '\n'
        subgroup_text += '\n'
        subgroup_text += print_group_options(subgroup)
        subgroup_text += print_subgroups(subgroup, depth + 1)
      return subgroup_text

    # Before printing named option groups, print any command-line options that were not
    #   explicitly placed into a group (the ungrouped 'OPTIONS' group).
    if self._ungrouped.options or self._ungrouped.subgroups:
      text += bold('OPTIONS') + '\n'
      text += '\n'
      text += print_group_options(self._ungrouped)
      text += print_subgroups(self._ungrouped, 0)
    # Named option groups, in reverse order of definition (matching prior behaviour);
    #   within each group, sub-groups render in declaration order after the group's options.
    for group in reversed(self._option_groups[1:]):
      if group.options or group.subgroups:
        text += bold(group.name) + '\n'
        text += '\n'
        text += print_group_options(group)
        text += print_subgroups(group, 0)
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

  # -------------------------------------------------------------------------------------
  # Machine-readable export formats.
  #   Stage 1 of the CLI overhaul deliberately leaves the four export special functions as
  #   placeholders; each is reimplemented, and verified byte-compatible with the captured
  #   baseline, one per stage (stages 2-5):
  #     __print_full_usage__     -> stage 2
  #     __print_synopsis__       -> stage 3
  #     __print_usage_markdown__ -> stage 4
  #     __print_usage_rst__      -> stage 5
  #   The native data model above (positional arguments, option groups, and all metadata
  #   slots) already carries everything these exporters will need to render.
  # -------------------------------------------------------------------------------------

  def _export_placeholder(self, keyword, stage):
    sys.stderr.write(f'{self.prog}: {ANSI.error}[ERROR] machine-readable export '
                     f'"{keyword}" is not implemented in this build; its reimplementation '
                     f'is scheduled for stage {stage} of the MRtrix3 CLI overhaul '
                     f'(retire-argparse effort){ANSI.clear}\n')
    sys.stderr.flush()

  def print_full_usage(self):
    # A per-algorithm interface is requested as "<command> <algorithm> __print_full_usage__";
    #   dispatch to that algorithm's sub-parser and emit its interface alone.
    if self._subparsers is not None and len(sys.argv) >= 3 and sys.argv[-2] in self._subparsers.choices:
      self._subparsers.choices[sys.argv[-2]].print_full_usage()
      return
    sys.stdout.write(f'{self._synopsis}\n')
    for line in self._description:
      sys.stdout.write(f'{line}\n')
    for example in self._examples:
      sys.stdout.write(f'{example[0]}: $ {example[1]}')
      if example[2]:
        sys.stdout.write(f'; {example[2]}')
      sys.stdout.write('\n')

    # Machine-readable type token for a single argument slot (positional Argument or an
    #   Option's argument slot). Mirrors the field semantics of the pre-overhaul argparse
    #   walker: choices take precedence, builtin int/float/str/None map to fixed strings,
    #   and CustomTypeBase instances defer to their _legacytypestring().
    def arg2str(spec):
      if spec.choices:
        return f'CHOICE {" ".join(spec.choices)}'
      argtype = spec.argtype
      if argtype is int:
        return f'INT {-sys.maxsize - 1} {sys.maxsize}'
      if argtype is float:
        return 'FLOAT -inf inf'
      if argtype is str or argtype is None:
        return 'TEXT'
      if isinstance(argtype, Parser.CustomTypeBase):
        return type(argtype)._legacytypestring() # pylint: disable=protected-access
      return argtype._legacytypestring() # pylint: disable=protected-access

    # For a multi-algorithm command, the sole positional is the algorithm selection,
    #   emitted as a CHOICE argument enumerating the available algorithm names; otherwise
    #   the command's own positional arguments are emitted. The allow_multiple field is 1
    #   only for variable-count positionals (former nargs='+'/'*'); "optional" is always 0.
    if self._subparsers is not None:
      sys.stdout.write(f'ARGUMENT {self._subparsers.dest} 0 0 CHOICE {" ".join(self._subparsers.choices)}\n')
    else:
      for argument in self._positional_args:
        allow_multiple = '1' if argument.allow_multiple else '0'
        sys.stdout.write(f'ARGUMENT {argument.name} 0 {allow_multiple} {arg2str(argument)}\n')
        sys.stdout.write(f'{Parser._fullusage_desc(argument.help, argument.default_value)}\n')

    # Options: the required field is inverted (0 if required, else 1); the option-level
    #   allow_multiple field is always 0 (repeatable/append options are NOT flagged here);
    #   one ARGUMENT line per argument slot, using each slot's metavar (falling back to the
    #   option name) and its own type token.
    # full_usage is deliberately flat: it carries no group headings (it never encoded even
    #   flat groups), so nesting adds nothing structurally; every option across the whole
    #   subtree is emitted via all_options() (own options first, then each sub-group's), in
    #   the same depth-first order the terminal/markdown/RST exports render them.
    def print_group_options(group):
      for option in group.all_options():
        required = '0' if option.required else '1'
        sys.stdout.write(f'OPTION -{option.name} {required} 0\n')
        # A scalar option's default value is preserved on the OPTION description line, exactly where
        #   the prose used to carry it before it was declared via set_default() (no dedicated machine
        #   token: the choice / range tokens remain on the ARGUMENT lines, so bash completion is
        #   unaffected). A tuple field's own description / default stays on its own argument line.
        scalar_arg = option.args[0] if len(option.args) == 1 and not option.args[0].is_tuple else None
        option_default = scalar_arg.default_value if scalar_arg is not None else None
        sys.stdout.write(f'{Parser._fullusage_desc(option.help, option_default)}\n')
        for arg in option.args:
          for leaf in arg.leaves():
            metavar_string = leaf.metavar if leaf.metavar else option.name
            sys.stdout.write(f'ARGUMENT {metavar_string} 0 0 {arg2str(leaf)}\n')
            field_desc = Parser._fullusage_desc(leaf.help, leaf.default_value) if arg.is_tuple else ''
            if field_desc:
              sys.stdout.write(f'{field_desc}\n')

    # Ungrouped options first (no heading), then the named groups in reverse order of
    #   definition, matching the terminal help traversal and the pre-overhaul baseline.
    if self._ungrouped.options:
      print_group_options(self._ungrouped)
    for group in reversed(self._option_groups[1:]):
      print_group_options(group)
    sys.stdout.flush()

  def print_synopsis(self):
    # Emit the stored synopsis string verbatim, with no trailing newline (matching the
    #   pre-overhaul baseline that inline-wrote CMDLINE._synopsis).
    sys.stdout.write(self._synopsis)
    sys.stdout.flush()

  def print_usage_markdown(self):
    # A per-algorithm interface is requested as "<command> <algorithm> __print_usage_markdown__";
    #   dispatch to that algorithm's sub-parser and emit its interface alone.
    if self._subparsers is not None and len(sys.argv) >= 3 and sys.argv[-2] in self._subparsers.choices:
      self._subparsers.choices[sys.argv[-2]].print_usage_markdown()
      return
    text = '## Synopsis\n\n'
    text += f'{self._synopsis}\n\n'
    text += '## Usage\n\n'
    text += f'    {self.format_usage()}\n\n'
    # For a multi-algorithm command the algorithm selection is described in place of any
    #   positional arguments (of which the top-level command has none).
    if self._subparsers is not None:
      text += f'-  *{self._subparsers.dest}*: {self._subparsers.help}\n'
    for argument in self._positional_args:
      name = argument.metavar if argument.metavar else argument.name
      text += f'-  *{name}*: {argument.help}{argument.help_metadata()}\n\n'
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

    # Render one option group as markdown.
    #   The double dash in the emitted option name (e.g. "--grad") is a faithful
    #   reproduction of the pre-overhaul baseline: the former renderer joined the
    #   already-dashed option_strings ("-grad") and prefixed a further "-".
    def print_group_options(group):
      group_text = ''
      for option in group.options:
        option_text = '-' + option.name + Parser._option_metavar(option)
        option_text = option_text.replace('<', '\\<').replace('>', '\\>')
        group_text += f'+ **-{option_text}**'
        if option.repeatable:
          group_text += '  *(multiple uses permitted)*'
        group_text += f'<br>{option.help}{option.help_metadata()}\n\n'
        # Described tuple fields render as an indented markdown sub-list beneath the option; a
        #   field's own choice / range / default metadata is appended to its description.
        field_lines = [f'    - *{Parser._leaf_id(leaf)}*: {leaf.help}{leaf.help_metadata()}'
                       for arg in option.args if arg.is_tuple
                       for leaf in arg.elements if leaf.help or leaf.help_metadata()]
        if field_lines:
          group_text += '\n'.join(field_lines) + '\n'
      return group_text

    # A nested child group renders as a deeper Markdown heading (one extra '#' per level of
    #   depth), then its own options, then recursively its own child groups. "depth" is 0 for
    #   a top-level group's sub-groups, i.e. heading level "#####".
    def print_subgroups(group, depth):
      subgroup_text = ''
      for subgroup in group.subgroups:
        subgroup_text += f'{"#" * (5 + depth)} {subgroup.name}\n\n'
        subgroup_text += print_group_options(subgroup)
        subgroup_text += print_subgroups(subgroup, depth + 1)
      return subgroup_text

    # Ungrouped options first (no heading), then the named groups in reverse order of
    #   definition, matching the terminal help traversal and the pre-overhaul baseline;
    #   within each group, sub-groups render in declaration order after the group's options.
    if self._ungrouped.options or self._ungrouped.subgroups:
      text += print_group_options(self._ungrouped)
      text += print_subgroups(self._ungrouped, 0)
    for group in reversed(self._option_groups[1:]):
      if group.options or group.subgroups:
        text += f'#### {group.name}\n\n'
        text += print_group_options(group)
        text += print_subgroups(group, 0)
    text += '## References\n\n'
    for entry in self._citation_list:
      ref_text = ''
      if entry[0]:
        ref_text += f'{entry[0]}: '
      ref_text += entry[1]
      text += f'{ref_text}\n\n'
    text += f'{_MRTRIX3_CORE_REFERENCE}\n\n'
    text += '---\n\n'
    text += f'**Author:** {self._author}\n\n'
    text += f'**Copyright:** {self._copyright}\n\n'
    sys.stdout.write(text)
    sys.stdout.flush()
    # Append one complete section per algorithm, reproducing the pre-overhaul behaviour of
    #   re-invoking the executable once per algorithm (done here in-process on the model).
    if self._subparsers is not None:
      for child in self._subparsers.choices.values():
        child.print_usage_markdown()

  def print_usage_rst(self):
    # A per-algorithm interface is requested as "<command> <algorithm> __print_usage_rst__";
    #   dispatch to that algorithm's sub-parser and emit its interface alone.
    if self._subparsers is not None and len(sys.argv) >= 3 and sys.argv[-2] in self._subparsers.choices:
      self._subparsers.choices[sys.argv[-2]].print_usage_rst()
      return
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
    # For a multi-algorithm command the algorithm selection is described in place of any
    #   positional arguments (of which the top-level command has none).
    if self._subparsers is not None:
      text += f'-  *{self._subparsers.dest}*: {self._subparsers.help}\n'
    for argument in self._positional_args:
      name = argument.metavar if argument.metavar else argument.name
      arg_help = (argument.help + argument.help_metadata()).replace('|', '\\|')
      text += f'-  *{name}*: {arg_help}\n'
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

    # Render one option group as RST.
    #   Each option is preceded by a blank line; a single-spelling option renders as
    #   "-name", its per-slot metavars follow, and repeatable (append) options are
    #   annotated with "*(multiple uses permitted)*". Pipe characters in help text are
    #   escaped for RST inline-markup safety, matching the pre-overhaul baseline.
    def print_group_options(group):
      group_text = ''
      for option in group.options:
        option_text = '-' + option.name + Parser._option_metavar(option)
        group_text += '\n'
        group_text += f'- **{option_text}**'
        if option.repeatable:
          group_text += '  *(multiple uses permitted)*'
        option_help = (option.help + option.help_metadata()).replace('|', '\\|')
        group_text += f' {option_help}'
        # Described tuple fields follow the summary on " |br|" continuation lines, matching
        #   the C++ RST rendering; a field's own choice / range / default metadata is appended
        #   to its description (scalar options add nothing here).
        field_lines = [ ]
        for arg in option.args:
          if not arg.is_tuple:
            continue
          for leaf in arg.elements:
            leaf_desc = leaf.help + leaf.help_metadata()
            if leaf_desc:
              leaf_help = leaf_desc.replace('|', '\\|')
              field_lines.append(f'   *{Parser._leaf_id(leaf)}*: {leaf_help}')
        if field_lines:
          group_text += ' |br|\n' + ' |br|\n'.join(field_lines)
        group_text += '\n'
      return group_text

    # A nested child group descends one further RST heading level per depth, its underline
    #   character chosen so Sphinx infers the correct nesting: depth 0 -> '"', 1 -> "'",
    #   2+ -> '~' ("depth" being 0 for a top-level group's sub-groups). Own options first,
    #   then recursively the child's own sub-groups.
    def subgroup_underline(depth):
      return '"\'~'[min(depth, 2)]
    def print_subgroups(group, depth):
      subgroup_text = ''
      for subgroup in group.subgroups:
        subgroup_text += '\n'
        subgroup_text += f'{subgroup.name}\n'
        subgroup_text += f'{subgroup_underline(depth) * len(subgroup.name)}\n'
        subgroup_text += print_group_options(subgroup)
        subgroup_text += print_subgroups(subgroup, depth + 1)
      return subgroup_text

    # Ungrouped options first (no heading), then the named groups in reverse order of
    #   definition, matching the terminal help traversal and the pre-overhaul baseline;
    #   within each group, sub-groups render in declaration order after the group's options.
    if self._ungrouped.options or self._ungrouped.subgroups:
      text += print_group_options(self._ungrouped)
      text += print_subgroups(self._ungrouped, 0)
    for group in reversed(self._option_groups[1:]):
      if group.options or group.subgroups:
        text += '\n'
        text += f'{group.name}\n'
        text += f'{"^"*len(group.name)}\n'
        text += print_group_options(group)
        text += print_subgroups(group, 0)
    text += '\n'
    text += 'References\n'
    text += '^^^^^^^^^^\n\n'
    for entry in self._citation_list:
      ref_text = '* '
      if entry[0]:
        ref_text += f'{entry[0]}: '
      ref_text += entry[1]
      text += f'{ref_text}\n\n'
    text += f'{_MRTRIX3_CORE_REFERENCE}\n\n'
    text += '--------------\n\n\n\n'
    text += f'**Author:** {self._author}\n\n'
    text += f'**Copyright:** {self._copyright}\n\n'
    sys.stdout.write(text)
    sys.stdout.flush()
    # Append one complete section per algorithm, reproducing the pre-overhaul behaviour of
    #   re-invoking the executable once per algorithm (done here in-process on the model).
    if self._subparsers is not None:
      for child in self._subparsers.choices.values():
        child.print_usage_rst()

  def print_version(self):
    text = f'== {self.prog} {self._git_version if self._is_project else version.VERSION} ==\n'
    if self._is_project:
      text += f'executing against MRtrix {version.VERSION}\n'
    text += f'Author(s): {self._author}\n'
    text += f'{self._copyright}\n'
    sys.stdout.write(text)
    sys.stdout.flush()



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
  # The two import formats are alternatives: at most one may be specified (reinstates the
  #   ad-hoc mutex removed in stage 1, now as a group constraint).
  options.mutually_exclusive()

def dwgrad_import_options(): #pylint: disable=unused-variable
  assert ARGS
  if ARGS.grad:
    return ['-grad', ARGS.grad]
  if ARGS.fslgrad:
    return ['-fslgrad', ARGS.fslgrad['bvecs'], ARGS.fslgrad['bvals']]
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
                       metavar=('bvecs_path', 'bvals_path'),
                       help='Export the final gradient table in FSL bvecs/bvals format')
  # The two export formats are alternatives: at most one may be specified (reinstates the
  #   ad-hoc mutex removed in stage 1, now as a group constraint).
  options.mutually_exclusive()

def dwgrad_export_options(): #pylint: disable=unused-variable
  assert ARGS
  if ARGS.export_grad_mrtrix:
    return ['-export_grad_mrtrix', ARGS.export_grad_mrtrix]
  if ARGS.export_grad_fsl:
    return ['-export_grad_fsl', ARGS.export_grad_fsl['bvecs_path'], ARGS.export_grad_fsl['bvals_path']]
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
