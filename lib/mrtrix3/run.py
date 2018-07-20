import os
_mrtrix_bin_path = os.path.abspath(os.path.join(os.path.abspath(os.path.dirname(os.path.abspath(__file__))), os.pardir, os.pardir, 'bin'))
# Remember to remove the '.exe' from Windows binary executables
_mrtrix_exe_list = [ os.path.splitext(name)[0] for name in os.listdir(_mrtrix_bin_path) ]

# If the main script has been executed in an SGE environment, don't allow
#   sub-processes to themselves fork SGE jobs; but if the main script is
#   itself not an SGE job ('JOB_ID' environment variable absent), then
#   whatever run.command() executes can send out SGE jobs without a problem.
_env = os.environ.copy()
if _env.get('SGE_ROOT') and _env.get('JOB_ID'):
  del _env['SGE_ROOT']

_processes = [ ]

_lastFile = ''



def setContinue(filename): #pylint: disable=unused-variable
  global _lastFile
  _lastFile = filename




def command(cmd, exitOnError=True): #pylint: disable=unused-variable

  import inspect, itertools, shlex, signal, string, subprocess, sys, tempfile
  from distutils.spawn import find_executable
  from mrtrix3 import app

  # This is the only global variable that is _modified_ within this function
  global _processes

  # Vectorise the command string, preserving anything encased within quotation marks
  if os.sep == '/': # Cheap POSIX compliance check
    cmdsplit = shlex.split(cmd)
  else: # Native Windows Python
    cmdsplit = [ entry.strip('\"') for entry in shlex.split(cmd, posix=False) ]

  if _lastFile:
    if _triggerContinue(cmdsplit):
      app.debug('Detected last file in command \'' + cmd + '\'; this is the last run.command() / run.function() call that will be skipped')
    if app.verbosity:
      sys.stderr.write(app.colourExec + 'Skipping command:' + app.colourClear + ' ' + cmd + '\n')
      sys.stderr.flush()
    return ('', '')

  # This splits the command string based on the piping character '|', such that each
  #   individual executable (along with its arguments) appears as its own list
  cmdstack = [ list(g) for k, g in itertools.groupby(cmdsplit, lambda s : s != '|') if k ]

  for line in cmdstack:
    is_mrtrix_exe = line[0] in _mrtrix_exe_list
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

  app.debug('To execute: ' + str(cmdstack))

  if app.verbosity:
    # Hide use of the hidden option used in mrconvert to alter header key-values and command history
    if '-copy_properties' in cmdsplit:
      index = cmdsplit.index('-copy_properties')
      del cmdsplit[index:index+5]
    sys.stderr.write(app.colourExec + 'Command:' + app.colourClear + '  ' + ' '.join(cmdsplit) + '\n')
    sys.stderr.flush()

  # Disable interrupt signal handler while threads are running
  try:
    signal.signal(signal.SIGINT, signal.default_int_handler)
  except:
    pass

  # Construct temporary text files for holding stdout / stderr contents when appropriate
  #   (One entry per process; each is a tuple containing two entries, each of which is either a
  #   file-like object, or None)
  tempfiles = [ ]

  # Execute all processes
  assert not _processes
  for index, to_execute in enumerate(cmdstack):
    file_out = None
    file_err = None
    # If there's at least one command prior to this, need to receive the stdout from the prior command
    #   at the stdin of this command; otherwise, nothing to receive
    if index > 0:
      handle_in = _processes[index-1].stdout
    else:
      handle_in = None
    # If this is not the last command, then stdout needs to be piped to the next command;
    #   otherwise, write stdout to a temporary file so that the contents can be read later
    if index < len(cmdstack)-1:
      handle_out = subprocess.PIPE
    else:
      file_out = tempfile.TemporaryFile()
      handle_out = file_out.fileno()
    # If we're in debug / info mode, the contents of stderr will be read and printed to the terminal
    #   as the command progresses, hence this needs to go to a pipe; otherwise, write it to a temporary
    #   file so that the contents can be read later
    if app.verbosity > 1:
      handle_err = subprocess.PIPE
    else:
      file_err = tempfile.TemporaryFile()
      handle_err = file_err.fileno()
    # Set off the processes
    try:
      try:
        process = subprocess.Popen (to_execute, stdin=handle_in, stdout=handle_out, stderr=handle_err, env=_env, preexec_fn=os.setpgrp)
      except AttributeError:
        process = subprocess.Popen (to_execute, stdin=handle_in, stdout=handle_out, stderr=handle_err, env=_env)
      _processes.append(process)
      tempfiles.append( ( file_out, file_err ) )
    # FileNotFoundError not defined in Python 2.7
    except OSError as e:
      if exitOnError:
        app.error('\'' + to_execute[0] + '\' not executed ("' + str(e) + '"); script cannot proceed')
      else:
        app.warn('\'' + to_execute[0] + '\' not executed ("' + str(e) + '")')
        for p in _processes:
          p.terminate()
        _processes = [ ]
        break

  return_stdout = ''
  return_stderr = ''
  error = False
  error_text = ''

  # Wait for all commands to complete
  # Switch how we monitor running processes / wait for them to complete
  #   depending on whether or not the user has specified -info or -debug option
  try:
    if app.verbosity > 1:
      for process in _processes:
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
          elif char == '\r' or char == '\n':
            do_indent = True
          sys.stderr.write(char)
          sys.stderr.flush()
        stderrdata = stderrdata.decode('utf-8', errors='replace')
        return_stderr += stderrdata
        if process.returncode:
          error = True
          error_text += stderrdata
    else:
      for process in _processes:
        process.wait()
  except (KeyboardInterrupt, SystemExit):
    app.handler(signal.SIGINT, inspect.currentframe())

  # Re-enable interrupt signal handler
  try:
    signal.signal(signal.SIGINT, app.handler)
  except:
    pass

  # For any command stdout / stderr data that wasn't either passed to another command or
  #   printed to the terminal during execution, read it here.
  for index in range(len(cmdstack)):
    if tempfiles[index][0] is not None:
      tempfiles[index][0].flush()
      tempfiles[index][0].seek(0)
      stdout_text = tempfiles[index][0].read().decode('utf-8', errors='replace')
      return_stdout += stdout_text
      if _processes[index].returncode:
        error = True
        error_text += stdout_text
    if tempfiles[index][1] is not None:
      tempfiles[index][1].flush()
      tempfiles[index][1].seek(0)
      stderr_text = tempfiles[index][1].read().decode('utf-8', errors='replace')
      return_stderr += stderr_text
      if _processes[index].returncode:
        error = True
        error_text += stderr_text

  _processes = [ ]

  if error:
    app.cleanup = False
    if exitOnError:
      caller = inspect.getframeinfo(inspect.stack()[1][0])
      script_name = os.path.basename(sys.argv[0])
      app.console('')
      try:
        filename = caller.filename
        lineno = caller.lineno
      except AttributeError:
        filename = caller[1]
        lineno = caller[2]
      sys.stderr.write(script_name + ': ' + app.colourError + '[ERROR] Command failed: ' + cmd + app.colourClear + app.colourDebug + ' (' + os.path.basename(filename) + ':' + str(lineno) + ')' + app.colourClear + '\n')
      if error_text:
        sys.stderr.write(script_name + ': ' + app.colourConsole + 'Output of failed command:' + app.colourClear + '\n')
        for line in error_text.splitlines():
          sys.stderr.write(' ' * (len(script_name)+2) + line + '\n')
      else:
        sys.stderr.write(script_name + ': ' + app.colourConsole + 'Failed command did not provide any diagnostic information' + app.colourClear + '\n')
      sys.stderr.flush()
      app.console('')
      if app.tempDir:
        with open(os.path.join(app.tempDir, 'error.txt'), 'w') as outfile:
          outfile.write(cmd + '\n\n' + error_text + '\n')
      app.complete()
      sys.exit(1)
    else:
      app.warn('Command failed: ' + cmd)

  # Only now do we append to the script log, since the command has completed successfully
  # Note: Writing the command as it was formed as the input to run.command():
  #   other flags may potentially change if this file is eventually used to resume the script
  if app.tempDir:
    with open(os.path.join(app.tempDir, 'log.txt'), 'a') as outfile:
      outfile.write(cmd + '\n')

  return (return_stdout, return_stderr)





def function(fn, *args, **kwargs): #pylint: disable=unused-variable

  import inspect, sys
  from mrtrix3 import app

  fnstring = fn.__module__ + '.' + fn.__name__ + \
             '(' + ', '.join(['\'' + str(a) + '\'' if isinstance(a, str) else str(a) for a in args]) + \
             (', ' if (args and kwargs) else '') + \
             ', '.join([key+'='+str(value) for key, value in kwargs.items()]) + ')'

  if _lastFile:
    if _triggerContinue(args) or _triggerContinue(kwargs.values()):
      app.debug('Detected last file in function \'' + fnstring + '\'; this is the last run.command() / run.function() call that will be skipped')
    if app.verbosity:
      sys.stderr.write(app.colourExec + 'Skipping function:' + app.colourClear + ' ' + fnstring + '\n')
      sys.stderr.flush()
    return None

  if app.verbosity:
    sys.stderr.write(app.colourExec + 'Function:' + app.colourClear + ' ' + fnstring + '\n')
    sys.stderr.flush()

  # Now we need to actually execute the requested function
  try:
    if kwargs:
      result = fn(*args, **kwargs)
    else:
      result = fn(*args)
  except (KeyboardInterrupt, SystemExit):
    raise
  except Exception as e: # pylint: disable=broad-except
    app.cleanup = False
    caller = inspect.getframeinfo(inspect.stack()[1][0])
    error_text = str(type(e).__name__) + ': ' + str(e)
    script_name = os.path.basename(sys.argv[0])
    app.console('')
    try:
      filename = caller.filename
      lineno = caller.lineno
    except AttributeError:
      filename = caller[1]
      lineno = caller[2]
    sys.stderr.write(script_name + ': ' + app.colourError + '[ERROR] Function failed: ' + fnstring + app.colourClear + app.colourDebug + ' (' + os.path.basename(filename) + ':' + str(lineno) + ')' + app.colourClear + '\n')
    sys.stderr.write(script_name + ': ' + app.colourConsole + 'Information from failed function:' + app.colourClear + '\n')
    for line in error_text.splitlines():
      sys.stderr.write(' ' * (len(script_name)+2) + line + '\n')
    app.console('')
    sys.stderr.flush()
    if app.tempDir:
      with open(os.path.join(app.tempDir, 'error.txt'), 'w') as outfile:
        outfile.write(fnstring + '\n\n' + error_text + '\n')
    app.complete()
    sys.exit(1)

  # Only now do we append to the script log, since the function has completed successfully
  if app.tempDir:
    with open(os.path.join(app.tempDir, 'log.txt'), 'a') as outfile:
      outfile.write(fnstring + '\n')

  return result







# When running on Windows, add the necessary '.exe' so that hopefully the correct
#   command is found by subprocess
def exeName(item):
  from distutils.spawn import find_executable
  from mrtrix3 import app
  global _mrtrix_bin_path
  if not app.isWindows():
    path = item
  elif item.endswith('.exe'):
    path = item
  elif os.path.isfile(os.path.join(_mrtrix_bin_path, item)):
    path = item
  elif os.path.isfile(os.path.join(_mrtrix_bin_path, item + '.exe')):
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



# Make sure we're not accidentally running an MRtrix executable on the system that
#   belongs to a different version of MRtrix3 to the script library currently being used,
#   or a non-MRtrix3 command with the same name as an MRtrix3 command
#   (e.g. C:\Windows\system32\mrinfo.exe; On Windows, subprocess uses CreateProcess(),
#   which checks system32\ before PATH)
def versionMatch(item):
  from distutils.spawn import find_executable
  from mrtrix3 import app
  global _mrtrix_bin_path, _mrtrix_exe_list

  if not item in _mrtrix_exe_list:
    app.debug('Command ' + item + ' not found in MRtrix3 bin/ directory')
    return item

  exe_path_manual = os.path.join(_mrtrix_bin_path, exeName(item))
  if os.path.isfile(exe_path_manual):
    app.debug('Version-matched executable for ' + item + ': ' + exe_path_manual)
    return exe_path_manual

  exe_path_sys = find_executable(exeName(item))
  if exe_path_sys and os.path.isfile(exe_path_sys):
    app.debug('Using non-version-matched executable for ' + item + ': ' + exe_path_sys)
    return exe_path_sys

  app.error('Unable to find executable for MRtrix3 command ' + item)
  return ''



# If the target executable is not a binary, but is actually a script, use the
#   shebang at the start of the file to alter the subprocess call
def _shebang(item):
  from mrtrix3 import app
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
      if app.isWindows():
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
          app.error('Malformed shebang in file \"' + item + '\": \"' + line + '\"')
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
    if totest == _lastFile or totest == os.path.splitext(_lastFile)[0]:
      _lastFile = ''
      return True
  return False
