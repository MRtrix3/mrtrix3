import collections, threading, os
from mrtrix3 import MRtrixBaseException



# If the main script has been executed in an SGE environment, don't allow
#   sub-processes to themselves fork SGE jobs; but if the main script is
#   itself not an SGE job ('JOB_ID' environment variable absent), then
#   whatever run.command() executes can send out SGE jobs without a problem.
_env = os.environ.copy()
if _env.get('SGE_ROOT') and _env.get('JOB_ID'):
  del _env['SGE_ROOT']

# Flagged by calling the setContinue() function;
#   run.command() and run.function() calls will be skipped until one of the inputs to
#   these functions matches the given string
_lastFile = ''

_lock = threading.Lock()

# Store executing processes so that they can be killed appropriately on interrupt;
#   e.g. by the signal handler in the mrtrix3.app module
# Each sequential execution of run.command() either selects the first index for which the value is None,
#   or extends the length of the list by 1, and uses this index as a unique identifier (within its own lifetime);
#   each item is then itself a list of processes required for that command
_processes = [ ]

# Construct temporary text files for holding stdout / stderr contents when appropriate
# Same as _processes: Index according to unique identifier for command() call
#   Each item is then a list with one entry per process; each of these is then a tuple containing two entries,
#   each of which is either a file-like object or None)
_tempFiles = [ ]

# TODO Consider combining the above two into a class that would be more easily nuked



class MRtrixCmdException(MRtrixBaseException):
  def __init__(self, cmd, code, stdout, stderr):
    super(MRtrixCmdException, self).__init__('Command failed')
    self.command = cmd
    self.returncode = code
    self.stdout = stdout
    self.stderr = stderr
  def __str__(self):
    return self.stdout + self.stderr

class MRtrixFnException(MRtrixBaseException):
  def __init__(self, fn, text):
    super(MRtrixFnException, self).__init__('Function failed')
    self.function = fn
    self.errortext = text
  def __str__(self):
    return self.errortext



CommandReturn = collections.namedtuple('CommandReturn', 'stdout stderr')



def setContinue(filename): #pylint: disable=unused-variable
  global _lastFile
  _lastFile = filename



def setTmpDir(path): #pylint: disable=unused-variable
  global _env
  _env['MRTRIX_TMPFILE_DIR'] = path



def command(cmd): #pylint: disable=unused-variable

  import inspect, itertools, shlex, signal, string, subprocess, sys, tempfile
  from distutils.spawn import find_executable
  from mrtrix3 import ansi, app, exe_list

  # These are the only global variables that are _modified_ within this function
  global _processes, _tempFiles

  if isinstance(cmd, list):
    cmdstring = ''
    cmdsplit = []
    for entry in cmd:
      if isinstance(entry, str):
        cmdstring += (' ' if cmdstring else '') + entry
        cmdsplit.append(entry)
      elif isinstance(entry, list):
        assert all([ isinstance(item, str) for item in entry ])
        if len(entry) > 3:
          common_prefix = os.path.commonprefix(entry)
          common_suffix = os.path.commonprefix([i[::-1] for i in entry])[::-1]
          cmdstring += (' ' if cmdstring else '') + '[ ' + common_prefix + '*' + common_suffix + ' (' + str(len(entry)) + ' entries) ]'
        else:
          cmdstring += (' ' if cmdstring else '') + ' '.join(entry)
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

  if _lastFile:
    if _triggerContinue(cmdsplit):
      app.debug('Detected last file in command \'' + cmdstring + '\'; this is the last run.command() / run.function() call that will be skipped')
    if app.verbosity:
      sys.stderr.write(ansi.execute + 'Skipping command:' + ansi.clear + ' ' + cmdstring + '\n')
      sys.stderr.flush()
    return CommandReturn(None, '', '')

  # Need to identify presence of list constructs && or ||, and process accordingly
  try:
    (index, operator) = next((i,v) for i,v in enumerate(cmdsplit) if v in [ '&&', '||' ])
    # If operator is '&&', next command should be executed only if first is successful
    # If operator is '||', next command should be executed only if the first is not successful
    try:
      pre_result = command(cmdsplit[:index])
      if operator == '||':
        app.debug('Due to success of "' + cmdsplit[:index] + '", "' + cmdsplit[index+1:] + '" will not be run')
        return pre_result
    except MRtrixCmdException:
      if operator == '&&':
        raise
    return command(cmdsplit[index+1:])
  except StopIteration:
    pass

  # This splits the command string based on the piping character '|', such that each
  #   individual executable (along with its arguments) appears as its own list
  cmdstack = [ list(g) for k, g in itertools.groupby(cmdsplit, lambda s : s != '|') if k ]

  for line in cmdstack:
    is_mrtrix_exe = line[0] in exe_list
    if is_mrtrix_exe:
      line[0] = versionMatch(line[0])
      if app.numThreads is not None:
        line.extend( [ '-nthreads', str(app.numThreads) ] )
      # Get MRtrix3 binaries to output additional INFO-level information if running in debug mode
      if app.verbosity == 3:
        line.append('-info')
      elif not app.verbosity:
        line.append('-quiet')
    else:
      line[0] = exeName(line[0])
    shebang = _shebang(line[0])
    if shebang:
      if not is_mrtrix_exe:
        # If a shebang is found, and this call is therefore invoking an
        #   interpreter, can't rely on the interpreter finding the script
        #   from PATH; need to find the full path ourselves.
        line[0] = find_executable(line[0])
      for item in reversed(shebang):
        line.insert(0, item)

  with _lock:
    app.debug('To execute: ' + str(cmdstack))
    if app.verbosity:
      # Hide use of these options in mrconvert to alter header key-values and command history at the end of scripts
      if all(key in cmdsplit for key in [ '-copy_properties', '-append_property', 'command_history' ]):
        index = cmdsplit.index('-append_property')
        del cmdsplit[index:index+3]
        index = cmdsplit.index('-copy_properties')
        del cmdsplit[index:index+2]
      sys.stderr.write(ansi.execute + 'Command:' + ansi.clear + '  ' + ' '.join(cmdsplit) + '\n')
      sys.stderr.flush()

    # Disable interrupt signal handler while threads are running
    # Only explicitly disable if there do not appear to be other commands running in parallel
    if not any(plist is not None for plist in _processes):
      try:
        signal.signal(signal.SIGINT, signal.default_int_handler)
      except:
        pass

    # Acquire a (temporarily) unique index for this command() call
    try:
      thisCommandIndex = next(i for i, v in enumerate(_processes) if v is None)
      _processes[thisCommandIndex] = [ ]
      _tempFiles[thisCommandIndex] = [ ]
    except StopIteration:
      thisCommandIndex = len(_processes)
      _processes.append([ ])
      _tempFiles.append([ ])

  # Execute all processes for this command
  thisProcesses = [ ]
  thisTempFiles = [ ]
  for index, to_execute in enumerate(cmdstack):
    file_out = None
    file_err = None
    # If there's at least one command prior to this, need to receive the stdout from the prior command
    #   at the stdin of this command; otherwise, nothing to receive
    if index > 0:
      handle_in = thisProcesses[index-1].stdout
    else:
      handle_in = None
    # If this is not the last command, then stdout needs to be piped to the next command;
    #   otherwise, write stdout to a temporary file so that the contents can be read later
    if index < len(cmdstack)-1:
      handle_out = subprocess.PIPE
    else:
      handle_out, file_out = tempfile.mkstemp()
    # If we're in debug / info mode, the contents of stderr will be read and printed to the terminal
    #   as the command progresses, hence this needs to go to a pipe; otherwise, write it to a temporary
    #   file so that the contents can be read later
    if app.verbosity > 1:
      handle_err = subprocess.PIPE
    else:
      handle_err, file_err = tempfile.mkstemp()
    # Set off the processes
    try:
      try:
        process = subprocess.Popen (to_execute, stdin=handle_in, stdout=handle_out, stderr=handle_err, env=_env, preexec_fn=os.setpgrp) # pylint: disable=bad-option-value,subprocess-popen-preexec-fn
      except AttributeError:
        process = subprocess.Popen (to_execute, stdin=handle_in, stdout=handle_out, stderr=handle_err, env=_env)
      thisProcesses.append(process)
      thisTempFiles.append( [ file_out, file_err ])
    # FileNotFoundError not defined in Python 2.7
    except OSError as e:
      raise MRtrixCmdException(cmdstring, 1, '', str(e))

  # Write process & temporary file information to globals
  with _lock:
    _processes[thisCommandIndex] = thisProcesses
    _tempFiles[thisCommandIndex] = thisTempFiles

  return_code = None
  return_stdout = ''
  return_stderr = ''
  error = False
  error_text = ''

  # Wait for all commands to complete
  # Switch how we monitor running processes / wait for them to complete
  #   depending on whether or not the user has specified -info or -debug option
  try:
    if app.verbosity > 1:
      for process in _processes[thisCommandIndex]:
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
      for process in _processes[thisCommandIndex]:
        process.wait()
        if not return_code:
          return_code = process.returncode
  except (KeyboardInterrupt, SystemExit):
    app.handler(signal.SIGINT, inspect.currentframe())

  # For any command stdout / stderr data that wasn't either passed to another command or
  #   printed to the terminal during execution, read it here.
  for index in range(len(cmdstack)):
    if thisTempFiles[index][0] is not None:
      with open(thisTempFiles[index][0], 'rb') as stdout_file:
        stdout_text = stdout_file.read().decode('utf-8', errors='replace')
      os.unlink(thisTempFiles[index][0])
      return_stdout += stdout_text
      if _processes[thisCommandIndex][index].returncode:
        error = True
        error_text += stdout_text
    if thisTempFiles[index][1] is not None:
      with open(thisTempFiles[index][1], 'rb') as stderr_file:
        stderr_text = stderr_file.read().decode('utf-8', errors='replace')
      os.unlink(thisTempFiles[index][1])
      return_stderr += stderr_text
      if _processes[thisCommandIndex][index].returncode:
        error = True
        error_text += stderr_text

  # Get rid of any reference to the executed processes
  thisProcesses = thisTempFiles = None
  with _lock:
    assert _processes[thisCommandIndex]
    _processes[thisCommandIndex] = None
    _tempFiles[thisCommandIndex] = None

    if error:
      raise MRtrixCmdException(cmdstring, return_code, stdout_text, stderr_text)

    # Re-enable interrupt signal handler
    #   Note: Only re-enable if there appears to not be any processes running in parallel from other commands
    if all(plist is None for plist in _processes):
      try:
        signal.signal(signal.SIGINT, app.handler)
      except:
        pass

    # Only now do we append to the script log, since the command has completed successfully
    # Note: Writing the command as it was formed as the input to run.command():
    #   other flags may potentially change if this file is eventually used to resume the script
    if app.tempDir:
      with open(os.path.join(app.tempDir, 'log.txt'), 'a') as outfile:
        outfile.write(cmdstring + '\n')

  return CommandReturn(return_stdout, return_stderr)





def function(fn, *args, **kwargs): #pylint: disable=unused-variable

  import sys
  from mrtrix3 import ansi, app

  fnstring = fn.__module__ + '.' + fn.__name__ + \
             '(' + ', '.join(['\'' + str(a) + '\'' if isinstance(a, str) else str(a) for a in args]) + \
             (', ' if (args and kwargs) else '') + \
             ', '.join([key+'='+str(value) for key, value in kwargs.items()]) + ')'

  if _lastFile:
    if _triggerContinue(args) or _triggerContinue(kwargs.values()):
      app.debug('Detected last file in function \'' + fnstring + '\'; this is the last run.command() / run.function() call that will be skipped')
    if app.verbosity:
      sys.stderr.write(ansi.execute + 'Skipping function:' + ansi.clear + ' ' + fnstring + '\n')
      sys.stderr.flush()
    return None

  if app.verbosity:
    sys.stderr.write(ansi.execute + 'Function:' + ansi.clear + ' ' + fnstring + '\n')
    sys.stderr.flush()

  # Now we need to actually execute the requested function
  try:
    if kwargs:
      result = fn(*args, **kwargs)
    else:
      result = fn(*args)
  except Exception as e: # pylint: disable=broad-except
    raise MRtrixFnException(fnstring, str(e))

  # Only now do we append to the script log, since the function has completed successfully
  _lock.acquire()
  if app.tempDir:
    with open(os.path.join(app.tempDir, 'log.txt'), 'a') as outfile:
      outfile.write(fnstring + '\n')
  _lock.release()

  return result







# When running on Windows, add the necessary '.exe' so that hopefully the correct
#   command is found by subprocess
def exeName(item):
  from distutils.spawn import find_executable
  from mrtrix3 import app, bin_path, isWindows
  if not isWindows():
    path = item
  elif item.endswith('.exe'):
    path = item
  elif os.path.isfile(os.path.join(bin_path, item)):
    path = item
  elif os.path.isfile(os.path.join(bin_path, item + '.exe')):
    path = item + '.exe'
  elif find_executable(item) is not None:
    path = item
  elif find_executable(item + '.exe') is not None:
    path = item + '.exe'
  # If it can't be found, return the item as-is; find_executable() fails to identify Python scripts
  else:
    path = item
  app.debug(item + ' -> ' + path)
  return path



# Kill any and all running processes
def killAll(): #pylint: disable=unused-variable
  global _processes, _tempFiles
  _lock.acquire()
  for plist in _processes:
    if plist:
      for p in plist:
        if p:
          p.terminate()
          p.communicate() # Flushes the I/O buffers
      plist = None
  _processes = [ ]
  # Also destroy any remaining temporary files
  for tlist in _tempFiles:
    for tpair in tlist:
      for t in tpair:
        if t:
          try:
            os.remove(t)
          except OSError:
            pass
      tpair = [ None, None ]
    tlist = [ ]
  _tempFiles = [ ]
  _lock.release()



# Make sure we're not accidentally running an MRtrix executable on the system that
#   belongs to a different version of MRtrix3 to the script library currently being used,
#   or a non-MRtrix3 command with the same name as an MRtrix3 command
#   (e.g. C:\Windows\system32\mrinfo.exe; On Windows, subprocess uses CreateProcess(),
#   which checks system32\ before PATH)
def versionMatch(item):
  from distutils.spawn import find_executable
  from mrtrix3 import app, bin_path, exe_list, MRtrixException

  if not item in exe_list:
    app.debug('Command ' + item + ' not found in MRtrix3 bin/ directory')
    return item

  exe_path_manual = os.path.join(bin_path, exeName(item))
  if os.path.isfile(exe_path_manual):
    app.debug('Version-matched executable for ' + item + ': ' + exe_path_manual)
    return exe_path_manual

  exe_path_sys = find_executable(exeName(item))
  if exe_path_sys and os.path.isfile(exe_path_sys):
    app.debug('Using non-version-matched executable for ' + item + ': ' + exe_path_sys)
    return exe_path_sys

  raise MRtrixException('Unable to find executable for MRtrix3 command ' + item)



# If the target executable is not a binary, but is actually a script, use the
#   shebang at the start of the file to alter the subprocess call
def _shebang(item):
  from mrtrix3 import app, isWindows, MRtrixException
  from distutils.spawn import find_executable
  # If a complete path has been provided rather than just a file name, don't perform any additional file search
  if os.sep in item:
    path = item
  else:
    path = versionMatch(item)
    if path == item:
      path = find_executable(exeName(item))
  if not path:
    app.debug('File \"' + item + '\": Could not find file to query')
    return []
  # Read the first 1024 bytes of the file
  with open(path, 'rb') as f:
    data = f.read(1024)
  # Try to find the shebang line
  for line in data.splitlines():
    # Are there any non-text characters? If so, it's a binary file, so no need to looking for a shebang
    try :
      line = str(line.decode('utf-8'))
    except:
      app.debug('File \"' + item + '\": Not a text file')
      return []
    line = line.strip()
    if len(line) > 2 and line[0:2] == '#!':
      # Need to strip first in case there's a gap between the shebang symbol and the interpreter path
      shebang = line[2:].strip().split(' ')
      if isWindows():
        # On Windows, /usr/bin/env can't be easily found, and any direct interpreter path will have a similar issue.
        #   Instead, manually find the right interpreter to call using distutils
        if os.path.basename(shebang[0]) == 'env':
          new_shebang = [ os.path.abspath(find_executable(exeName(shebang[1]))) ]
          new_shebang.extend(shebang[2:])
          shebang = new_shebang
        else:
          new_shebang = [ os.path.abspath(find_executable(exeName(os.path.basename(shebang[0])))) ]
          new_shebang.extend(shebang[1:])
          shebang = new_shebang
        if not shebang or not shebang[0]:
          raise MRtrixException('Malformed shebang in file \"' + item + '\": \"' + line + '\"')
      app.debug('File \"' + item + '\": string \"' + line + '\": ' + str(shebang))
      return shebang
  app.debug('File \"' + item + '\": No shebang found')
  return []



# New function for handling the -continue command-line option functionality
# Check to see if the last file produced in the previous script execution is
#   intended to be produced by this command; if it is, this will be the last
#   thing that gets skipped by the -continue option
def _triggerContinue(entries):
  global _lastFile
  if not _lastFile:
    assert False
  for entry in entries:
    # It's possible that the file might be defined in a '--option=XXX' style argument
    #   It's also possible that the filename in the command string has the file extension omitted
    if entry.startswith('--') and '=' in entry:
      totest = entry.split('=')[1]
    else:
      totest = entry
    if totest in [ _lastFile, os.path.splitext(_lastFile)[0] ]:
      _lastFile = ''
      return True
  return False
