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

import collections, itertools, os, shlex, shutil, signal, string, subprocess, sys, tempfile, threading
from mrtrix3 import ANSI, BIN_PATH, COMMAND_HISTORY_STRING, EXE_LIST, MRtrixBaseError, MRtrixError

IOStream = collections.namedtuple('IOStream', 'handle filename')



class Shared:

  # A class for storing all information related to a running process
  # This includes:
  # - The process itself (so that a terminate signal can be sent if required)
  # - For both stdout and stderr:
  #   - File handle (so that if not an internal pipe, the file can be closed)
  #   - File name (so that if not an internal pipe, it can be deleted from the file system)
  #   (these are encapsulated in named tuple "IOStream"
  # Note: Input "stdin" should be a stream handle only, or None;
  #   "stdout" and "stderr" should be of type "IOStream"
  class Process(subprocess.Popen):
    def __init__(self, cmd, stdin, stdout, stderr, **kwargs):
      assert isinstance(stdout, IOStream) or stdout is None
      assert isinstance(stderr, IOStream) or stderr is None
      my_kwargs = kwargs.copy()
      my_kwargs['stdin']  = stdin
      my_kwargs['stdout'] = stdout.handle if stdout else None
      my_kwargs['stderr'] = stderr.handle if stderr else None
      super(Shared.Process, self).__init__(cmd, **my_kwargs)
      self.iostreams = (stdout, stderr)



  def __init__(self):
    # If the main script has been executed in an SGE environment, don't allow
    #   sub-processes to themselves fork SGE jobs; but if the main script is
    #   itself not an SGE job ('JOB_ID' environment variable absent), then
    #   whatever run.command() executes can send out SGE jobs without a problem.
    self.env = os.environ.copy()
    if self.env.get('SGE_ROOT') and self.env.get('JOB_ID'):
      del self.env['SGE_ROOT']

    # If environment variable is set, should apply to invoked script,
    #   but not to any underlying invoked commands
    try:
      self.env.pop('MRTRIX_QUIET')
    except KeyError:
      pass
    self.env['MRTRIX_LOGLEVEL'] = '1'

    # Flagged by calling the set_continue() function;
    #   run.command() and run.function() calls will be skipped until one of the inputs to
    #   these functions matches the given string
    self._last_file = ''

    self.lock = threading.Lock()
    self._num_threads = None

    # Store executing processes so that they can be killed appropriately on interrupt;
    #   e.g. by the signal handler in the mrtrix3.app module
    # Each sequential execution of run.command() either selects the first index for which the value is None,
    #   or extends the length of the list by 1, and uses this index as a unique identifier (within its own lifetime);
    #   each item is then itself a list of Process class instances required for that command string
    self.process_lists = [ ]

    self._scratch_dir = None
    self.verbosity = 1

  # Acquire a unique index
  # This ensures that if command() is executed in parallel using different threads, they will
  #   not interfere with one another; but terminate() will also have access to all relevant data
  def get_command_index(self):
    with self.lock:
      try:
        index = next(i for i, v in enumerate(self.process_lists) if v is None)
        self.process_lists[index] = [ ]
      except StopIteration:
        index = len(self.process_lists)
        self.process_lists.append([ ])
    return index

  def close_command_index(self, index):
    with self.lock:
      assert index < len(self.process_lists)
      assert self.process_lists[index]
      self.process_lists[index] = None

  # Wrap tempfile.mkstemp() in a convenience function, which also catches the case
  #   where the user does not have write access to the temporary directory location
  #   selected by default by the tempfile module, and in that case re-runs mkstemp()
  #   manually specifying an alternative temporary directory
  def make_temporary_file(self):
    try:
      return IOStream(*tempfile.mkstemp())
    except OSError:
      return IOStream(*tempfile.mkstemp('', 'tmp', self._scratch_dir if self._scratch_dir else os.getcwd()))

  def set_continue(self, filename): #pylint: disable=unused-variable
    self._last_file = filename

  def get_continue(self):
    return bool(self._last_file)

  # New function for handling the -continue command-line option functionality
  # Check to see if the last file produced in the previous script execution is
  #   intended to be produced by this command; if it is, this will be the last
  #   thing that gets skipped by the -continue option
  def trigger_continue(self, entries):
    assert self.get_continue()
    for entry in entries:
      # It's possible that the file might be defined in a '--option=XXX' style argument
      #   It's also possible that the filename in the command string has the file extension omitted
      if entry.startswith('--') and '=' in entry:
        totest = entry.split('=')[1]
      else:
        totest = entry
      if totest in [ self._last_file, os.path.splitext(self._last_file)[0] ]:
        self._last_file = ''
        return True
    return False

  def get_num_threads(self):
    return self._num_threads

  def set_num_threads(self, value):
    assert value is None or (isinstance(value, int) and value >= 0)
    self._num_threads = value
    if value is not None:
      # Either -nthreads 0 or -nthreads 1 should result in disabling multi-threading
      external_software_value = 1 if value <= 1 else value
      self.env['ITK_GLOBAL_NUMBER_OF_THREADS'] = str(external_software_value)
      self.env['OMP_NUM_THREADS'] = str(external_software_value)

  def get_scratch_dir(self):
    return self._scratch_dir

  def set_scratch_dir(self, path):
    self.env['MRTRIX_TMPFILE_DIR'] = path
    self._scratch_dir = path

  # Controls verbosity of invoked MRtrix3 commands, as well as whether or not the
  #   stderr outputs of invoked commands are propagated to the terminal instantly or
  #   instead written to a temporary file for read on completion
  def set_verbosity(self, verbosity):
    assert isinstance(verbosity, int)
    self.verbosity = verbosity
    self.env['MRTRIX_LOGLEVEL'] = str(max(1, verbosity-1))

  # Terminate any and all running processes, and delete any associated temporary files
  def terminate(self, signum): #pylint: disable=unused-variable
    with self.lock:
      for process_list in self.process_lists:
        if process_list:
          for process in process_list:
            if process:
              # No need to propagate signals if we're in a POSIX-compliant environment
              #   and SIGINT has been received; that will propagate to children automatically
              if sys.platform == 'win32':
                process.send_signal(getattr(signal, 'CTRL_BREAK_EVENT'))
                process.communicate(timeout=1) # Flushes the I/O buffers, unlike wait()
              elif signum != signal.SIGINT:
                process.terminate()
                process.communicate(timeout=1)
            for stream in process.iostreams:
              if stream:
                if stream.handle != subprocess.PIPE:
                  try:
                    os.close(stream.handle)
                  except OSError:
                    pass
                if stream.filename is not None:
                  try:
                    os.remove(stream.filename)
                  except OSError:
                    pass
                stream = None
            process = None
          process_list = None
      self.process_lists = [ ]


shared = Shared() #pylint: disable=invalid-name



class MRtrixCmdError(MRtrixBaseError):
  def __init__(self, cmd, code, stdout, stderr):
    super().__init__('Command failed')
    self.command = cmd
    self.returncode = code
    self.stdout = stdout
    self.stderr = stderr
  def __str__(self):
    return self.stdout + self.stderr

class MRtrixFnError(MRtrixBaseError):
  def __init__(self, fn, text):
    super().__init__('Function failed')
    self.function = fn
    self.errortext = text
  def __str__(self):
    return self.errortext



CommandReturn = collections.namedtuple('CommandReturn', 'stdout stderr')



def command(cmd, **kwargs): #pylint: disable=unused-variable
  from mrtrix3 import app #pylint: disable=import-outside-toplevel

  def quote_nonpipe(item):
    return item if item == '|' else shlex.quote(item)

  shell = kwargs.pop('shell', False)
  show = kwargs.pop('show', True)
  mrconvert_keyval = kwargs.pop('mrconvert_keyval', None)
  force = kwargs.pop('force', False)
  env = kwargs.pop('env', shared.env)
  if kwargs:
    raise TypeError('Unsupported keyword arguments passed to run.command(): ' + str(kwargs))

  if shell and mrconvert_keyval:
    raise TypeError('Cannot use "mrconvert_keyval=" parameter in shell mode')

  subprocess_kwargs = {}
  if sys.platform == 'win32':
    subprocess_kwargs['creationflags'] = subprocess.CREATE_NEW_PROCESS_GROUP
  if shell:
    subprocess_kwargs['shell'] = True
  subprocess_kwargs['env'] = env

  if isinstance(cmd, list):
    if shell:
      raise TypeError('When using run.command() with shell=True, input must be a text string')
    cmdstring = ''
    cmdsplit = []
    for entry in cmd:
      if isinstance(entry, str):
        cmdstring += (' ' if cmdstring else '') + quote_nonpipe(entry)
        cmdsplit.append(entry)
      elif isinstance(entry, list):
        assert all(isinstance(item, str) for item in entry)
        if len(entry) > 1:
          common_prefix = os.path.commonprefix(entry)
          common_suffix = os.path.commonprefix([i[::-1] for i in entry])[::-1]
          if common_prefix == entry[0] and common_prefix == common_suffix:
            cmdstring += (' ' if cmdstring else '') + '[' + entry[0] + ' (x' + str(len(entry)) + ')]'
          else:
            cmdstring += (' ' if cmdstring else '') + '[' + common_prefix + '*' + common_suffix + ' (' + str(len(entry)) + ' items)]'
        else:
          cmdstring += (' ' if cmdstring else '') + quote_nonpipe(entry[0])
        cmdsplit.extend(entry)
      else:
        raise TypeError('When run.command() is provided with a list as input, entries in the list must be either strings or lists of strings')
  elif isinstance(cmd, str):
    cmdstring = cmd
    # Split the command string by spaces, preserving anything encased within quotation marks
    if os.sep == '/': # Cheap POSIX compliance check
      cmdsplit = shlex.split(cmd)
    else: # Native Windows Python
      cmdsplit = [ entry.strip('\"') for entry in shlex.split(cmd, posix=False) ]
  else:
    raise TypeError('run.command() function only operates on strings, or lists of strings')

  if shared.get_continue():
    if shared.trigger_continue(cmdsplit):
      app.debug('Detected last file in command \'' + cmdstring + '\'; this is the last run.command() / run.function() call that will be skipped')
    if shared.verbosity:
      sys.stderr.write(ANSI.execute + 'Skipping command:' + ANSI.clear + ' ' + cmdstring + '\n')
      sys.stderr.flush()
    return CommandReturn('', '')


  # If operating in shell=True mode, handling of command execution is significantly different:
  #   Unmodified command string is executed using subprocess, with the shell being responsible for its parsing
  #   Only a single process per run.command() invocation is possible (since e.g. any piping will be
  #     handled by the spawned shell)
  this_process_list = [ ]

  if shell:

    cmdstack = [ cmdsplit ]
    with shared.lock:
      app.debug('To execute: ' + str(cmdsplit))
      if (shared.verbosity and show) or shared.verbosity > 1:
        sys.stderr.write(ANSI.execute + 'Command:' + ANSI.clear + '  ' + cmdstring + '\n')
        sys.stderr.flush()
    # No locking required for actual creation of new process
    this_stdout = shared.make_temporary_file()
    this_stderr = IOStream(subprocess.PIPE, None) if shared.verbosity > 1 else shared.make_temporary_file()
    this_process_list.append(shared.Process(cmdstring, None, this_stdout, this_stderr, **subprocess_kwargs))

  else: # shell=False

    # Need to identify presence of list constructs && or ||, and process accordingly
    try:
      (index, operator) = next((i,v) for i,v in enumerate(cmdsplit) if v in [ '&&', '||' ])
      # If operator is '&&', next command should be executed only if first is successful
      # If operator is '||', next command should be executed only if the first is not successful
      try:
        pre_result = command(cmdsplit[:index])
        if operator == '||':
          with shared.lock:
            app.debug('Due to success of "' + cmdsplit[:index] + '", "' + cmdsplit[index+1:] + '" will not be run')
          return pre_result
      except MRtrixCmdError:
        if operator == '&&':
          raise
      return command(cmdsplit[index+1:])
    except StopIteration:
      pass

    # This splits the command string based on the piping character '|', such that each
    #   individual executable (along with its arguments) appears as its own list
    cmdstack = [ list(g) for k, g in itertools.groupby(cmdsplit, lambda s : s != '|') if k ]

    if mrconvert_keyval:
      if cmdstack[-1][0] != 'mrconvert':
        raise TypeError('Argument "mrconvert_keyval=" can only be used if the mrconvert command is being invoked')
      assert not (mrconvert_keyval[0] in [ '\'', '"' ] or mrconvert_keyval[-1] in [ '\'', '"' ])
      cmdstack[-1].extend([ '-copy_properties', mrconvert_keyval ])
      if COMMAND_HISTORY_STRING:
        cmdstack[-1].extend([ '-append_property', 'command_history', COMMAND_HISTORY_STRING ])

    for line in cmdstack:
      is_mrtrix_exe = line[0] in EXE_LIST
      if is_mrtrix_exe:
        line[0] = version_match(line[0])
        if shared.get_num_threads() is not None:
          line.extend( [ '-nthreads', str(shared.get_num_threads()) ] )
        if force:
          line.append('-force')
      else:
        line[0] = exe_name(line[0])
      shebang = _shebang(line[0])
      if shebang:
        if not is_mrtrix_exe:
          # If a shebang is found, and this call is therefore invoking an
          #   interpreter, can't rely on the interpreter finding the script
          #   from PATH; need to find the full path ourselves.
          line[0] = shutil.which(line[0])
        for item in reversed(shebang):
          line.insert(0, item)

    with shared.lock:
      app.debug('To execute: ' + str(cmdstack))
      if (shared.verbosity and show) or shared.verbosity > 1:
        sys.stderr.write(ANSI.execute + 'Command:' + ANSI.clear + '  ' + cmdstring + '\n')
        sys.stderr.flush()

    this_command_index = shared.get_command_index()

    # Execute all processes for this command string
    for index, to_execute in enumerate(cmdstack):
      # If there's at least one command prior to this, need to receive the stdout from the prior command
      #   at the stdin of this command; otherwise, nothing to receive
      this_stdin = this_process_list[index-1].stdout if index > 0 else None
      # If this is not the last command, then stdout needs to be piped to the next command;
      #   otherwise, write stdout to a temporary file so that the contents can be read later
      this_stdout = IOStream(subprocess.PIPE, None) if index<len(cmdstack)-1 else shared.make_temporary_file()
      # If we're in debug / info mode, the contents of stderr will be read and printed to the terminal
      #   as the command progresses, hence this needs to go to a pipe; otherwise, write it to a temporary
      #   file so that the contents can be read later
      this_stderr = IOStream(subprocess.PIPE, None) if shared.verbosity>1 else shared.make_temporary_file()
      # Set off the process
      try:
        this_process_list.append(shared.Process(to_execute, this_stdin, this_stdout, this_stderr, **subprocess_kwargs))
      except FileNotFoundError as exception:
        raise MRtrixCmdError(cmdstring, 1, '', str(exception)) from exception

  # End branching based on shell=True/False

  # Write process & temporary file information to globals, so that
  #   shared.terminate() can perform cleanup if required
  this_command_index = shared.get_command_index()
  with shared.lock:
    shared.process_lists[this_command_index] = this_process_list

  return_code = None
  return_stdout = ''
  return_stderr = ''
  error = False
  error_text = ''

  # Wait for all commands to complete
  # Switch how we monitor running processes / wait for them to complete
  #   depending on whether or not the user has specified -info or -debug option
  if shared.verbosity > 1:
    for process in this_process_list:
      stderrdata = b''
      do_indent = True
      while True:
        # Have to read one character at a time: Waiting for a newline character using e.g. readline() will prevent MRtrix progressbars from appearing
        byte = process.stderr.read(1)
        stderrdata += byte
        char = byte.decode('cp1252', errors='ignore')
        if not char and process.poll() is not None:
          break
        if do_indent and char in string.printable and char != '\r' and char != '\n':
          sys.stderr.write('          ')
          do_indent = False
        elif char in [ '\r', '\n' ]:
          do_indent = True
        sys.stderr.write(char)
        sys.stderr.flush()
      stderrdata = stderrdata.decode('utf-8', errors='replace')
      return_stderr += stderrdata
      if not return_code: # Catch return code of first failed command
        return_code = process.returncode
      if process.returncode:
        error = True
        error_text += stderrdata
  else:
    for process in this_process_list:
      process.wait()
      if not return_code:
        return_code = process.returncode

  # For any command stdout / stderr data that wasn't either passed to another command or
  #   printed to the terminal during execution, read it here.
  for process in this_process_list:

    def finalise_temp_file(iostream):
      os.close(iostream.handle)
      with open(iostream.filename, 'rb') as stream:
        contents = stream.read().decode('utf-8', errors='replace')
      os.unlink(iostream.filename)
      iostream = None
      return contents

    stdout_text = stderr_text = ''
    if process.iostreams[0].filename is not None:
      stdout_text = finalise_temp_file(process.iostreams[0])
      return_stdout += stdout_text
    if process.iostreams[1].filename is not None:
      stderr_text = finalise_temp_file(process.iostreams[1])
      return_stderr += stderr_text
    if process.returncode:
      error = True
      error_text += stdout_text + stderr_text

  # Get rid of any reference to the executed processes
  shared.close_command_index(this_command_index)
  this_process_list = None

  if error:
    raise MRtrixCmdError(cmdstring, return_code, return_stdout, return_stderr)

  # Only now do we append to the script log, since the command has completed successfully
  # Note: Writing the command as it was formed as the input to run.command():
  #   other flags may potentially change if this file is eventually used to resume the script
  if shared.get_scratch_dir():
    with shared.lock:
      with open(os.path.join(shared.get_scratch_dir(), 'log.txt'), 'a', encoding='utf-8') as outfile:
        outfile.write(cmdstring + '\n')

  return CommandReturn(return_stdout, return_stderr)





def function(fn_to_execute, *args, **kwargs): #pylint: disable=unused-variable
  from mrtrix3 import app #pylint: disable=import-outside-toplevel
  if not fn_to_execute:
    raise TypeError('Invalid input to run.function()')

  show = kwargs.pop('show', True)

  fnstring = fn_to_execute.__module__ + '.' + fn_to_execute.__name__ + \
             '(' + ', '.join(['\'' + str(a) + '\'' if isinstance(a, str) else str(a) for a in args]) + \
             (', ' if (args and kwargs) else '') + \
             ', '.join([key+'='+str(value) for key, value in kwargs.items()]) + ')'

  if shared.get_continue():
    if shared.trigger_continue(args) or shared.trigger_continue(kwargs.values()):
      app.debug('Detected last file in function \'' + fnstring + '\'; this is the last run.command() / run.function() call that will be skipped')
    if shared.verbosity:
      sys.stderr.write(ANSI.execute + 'Skipping function:' + ANSI.clear + ' ' + fnstring + '\n')
      sys.stderr.flush()
    return None

  if (shared.verbosity and show) or shared.verbosity > 1:
    sys.stderr.write(ANSI.execute + 'Function:' + ANSI.clear + ' ' + fnstring + '\n')
    sys.stderr.flush()

  # Now we need to actually execute the requested function
  try:
    if kwargs:
      result = fn_to_execute(*args, **kwargs)
    else:
      result = fn_to_execute(*args)
  except Exception as exception: # pylint: disable=broad-except
    raise MRtrixFnError(fnstring, str(exception)) from exception

  # Only now do we append to the script log, since the function has completed successfully
  if shared.get_scratch_dir():
    with shared.lock:
      with open(os.path.join(shared.get_scratch_dir(), 'log.txt'), 'a', encoding='utf-8') as outfile:
        outfile.write(fnstring + '\n')

  return result



# When running on Windows, add the necessary '.exe' so that hopefully the correct
#   command is found by subprocess
def exe_name(item):
  from mrtrix3 import app, utils #pylint: disable=import-outside-toplevel
  if not utils.is_windows():
    path = item
  elif item.endswith('.exe'):
    path = item
  elif os.path.isfile(os.path.join(BIN_PATH, item)):
    path = item
  elif os.path.isfile(os.path.join(BIN_PATH, item + '.exe')):
    path = item + '.exe'
  elif shutil.which(item) is not None:
    path = item
  elif shutil.which(item + '.exe') is not None:
    path = item + '.exe'
  # If it can't be found, return the item as-is
  else:
    path = item
  app.debug(item + ' -> ' + path)
  return path



# Make sure we're not accidentally running an MRtrix executable on the system that
#   belongs to a different version of MRtrix3 to the script library currently being used,
#   or a non-MRtrix3 command with the same name as an MRtrix3 command
#   (e.g. C:\Windows\system32\mrinfo.exe; On Windows, subprocess uses CreateProcess(),
#   which checks system32\ before PATH)
def version_match(item):
  from mrtrix3 import app #pylint: disable=import-outside-toplevel
  if not item in EXE_LIST:
    app.debug('Command ' + item + ' not found in MRtrix3 bin/ directory')
    return item
  exe_path_manual = os.path.join(BIN_PATH, exe_name(item))
  if os.path.isfile(exe_path_manual):
    app.debug('Version-matched executable for ' + item + ': ' + exe_path_manual)
    return exe_path_manual
  exe_path_sys = shutil.which(exe_name(item))
  if exe_path_sys and os.path.isfile(exe_path_sys):
    app.debug('Using non-version-matched executable for ' + item + ': ' + exe_path_sys)
    return exe_path_sys
  raise MRtrixError('Unable to find executable for MRtrix3 command ' + item)



# If the target executable is not a binary, but is actually a script, use the
#   shebang at the start of the file to alter the subprocess call
def _shebang(item):
  from mrtrix3 import app, utils #pylint: disable=import-outside-toplevel
  # If a complete path has been provided rather than just a file name, don't perform any additional file search
  if os.sep in item:
    path = item
  else:
    path = version_match(item)
    if path == item:
      path = shutil.which(exe_name(item))
  if not path:
    app.debug('File \"' + item + '\": Could not find file to query')
    return []
  # Read the first 1024 bytes of the file
  with open(path, 'rb') as file_in:
    data = file_in.read(1024)
  # Try to find the shebang line
  for line in data.splitlines():
    # Are there any non-text characters? If so, it's a binary file, so no need to looking for a shebang
    try:
      line = str(line.decode('utf-8'))
    except UnicodeDecodeError:
      app.debug('File \"' + item + '\": Not a text file')
      return []
    line = line.strip()
    if len(line) > 2 and line[0:2] == '#!':
      # Need to strip first in case there's a gap between the shebang symbol and the interpreter path
      shebang = line[2:].strip().split(' ')
      # On Windows, /usr/bin/env can't be easily found
      #   Instead, manually find the right interpreter to call using distutils
      # Also if script is written in Python, try to execute it using the same interpreter as that currently running,
      #   as long as Python2 is not explicitly requested
      if os.path.basename(shebang[0]) == 'env':
        if len(shebang) < 2:
          app.warn('Invalid shebang in script file \"' + item + '\" (missing interpreter after \"env\")')
          return []
        if shebang[1] == 'python' or shebang[1] == 'python3':
          if not sys.executable:
            app.warn('Unable to self-identify Python interpreter; file \"' + item + '\" not guaranteed to execute on same version')
            return []
          shebang = [ sys.executable ] + shebang[2:]
          app.debug('File \"' + item + '\": Using current Python interpreter')
        elif utils.is_windows():
          shebang = [ os.path.abspath(shutil.which(exe_name(shebang[1]))) ] + shebang[2:]
      elif utils.is_windows():
        shebang = [ os.path.abspath(shutil.which(exe_name(os.path.basename(shebang[0])))) ] + shebang[1:]
      app.debug('File \"' + item + '\": string \"' + line + '\": ' + str(shebang))
      return shebang
  app.debug('File \"' + item + '\": No shebang found')
  return []
