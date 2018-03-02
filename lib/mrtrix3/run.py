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



def command(cmd, exitOnError=True):

  import inspect, itertools, os, shlex, subprocess, sys, tempfile
  from distutils.spawn import find_executable
  from mrtrix3 import app

  # This is the only global variable that is _modified_ within this function
  global _processes

  # Vectorise the command string, preserving anything encased within quotation marks
  cmdsplit = shlex.split(cmd)

  if app._lastFile:
    # Check to see if the last file produced in the previous script execution is
    #   intended to be produced by this command; if it is, this will be the last
    #   command that gets skipped by the -continue option
    # It's possible that the file might be defined in a '--option=XXX' style argument
    #   It's also possible that the filename in the command string has the file extension omitted
    for entry in cmdsplit:
      if entry.startswith('--') and '=' in entry:
        cmdtotest = entry.split('=')[1]
      else:
        cmdtotest = entry
      filetotest = [ app._lastFile, os.path.splitext(app._lastFile)[0] ]
      if cmdtotest in filetotest:
        app.debug('Detected last file \'' + app._lastFile + '\' in command \'' + cmd + '\'; this is the last run.command() / run.function() call that will be skipped')
        app._lastFile = ''
        break
    if app._verbosity:
      sys.stderr.write(app.colourExec + 'Skipping command:' + app.colourClear + ' ' + cmd + '\n')
      sys.stderr.flush()
    return

  # This splits the command string based on the piping character '|', such that each
  #   individual executable (along with its arguments) appears as its own list
  # Note that for Python2 support, it is necessary to convert groupby() output from
  #   a generator to a list before it is passed to filter()
  cmdstack = [ list(g) for k, g in filter(lambda t : t[0], ((k, list(g)) for k, g in itertools.groupby(cmdsplit, lambda s : s is not '|') ) ) ]

  for line in cmdstack:
    is_mrtrix_exe = line[0] in _mrtrix_exe_list
    if is_mrtrix_exe:
      line[0] = versionMatch(line[0])
      if app._nthreads is not None:
        line.extend( [ '-nthreads', str(app._nthreads) ] )
      # Get MRtrix3 binaries to output additional INFO-level information if running in debug mode
      if app._verbosity == 3:
        line.append('-info')
      elif not app._verbosity:
        line.append('-quiet')
    else:
      line[0] = exeName(line[0])
    shebang = _shebang(line[0])
    if len(shebang):
      if not is_mrtrix_exe:
        # If a shebang is found, and this call is therefore invoking an
        #   interpreter, can't rely on the interpreter finding the script
        #   from PATH; need to find the full path ourselves.
        line[0] = find_executable(line[0])
      for item in reversed(shebang):
        line.insert(0, item)

  if app._verbosity:
    sys.stderr.write(app.colourExec + 'Command:' + app.colourClear + '  ' + cmd + '\n')
    sys.stderr.flush()

  app.debug('To execute: ' + str(cmdstack))

  # Construct temporary text files for holding stdout / stderr contents when appropriate
  #   (One entry per process; each is a tuple containing two entries, each of which is either a
  #   file-like object, or None)
  tempfiles = [ ]

  # Execute all processes
  _processes = [ ]
  for index, command in enumerate(cmdstack):
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
    if app._verbosity > 1:
      handle_err = subprocess.PIPE
    else:
      file_err = tempfile.TemporaryFile()
      handle_err = file_err.fileno()
    # Set off the processes
    try:
      process = subprocess.Popen (command, stdin=handle_in, stdout=handle_out, stderr=handle_err, env=_env)
      _processes.append(process)
      tempfiles.append( ( file_out, file_err ) )
    # FileNotFoundError not defined in Python 2.7
    except OSError as e:
      if exitOnError:
        app.error('\'' + command[0] + '\' not executed ("' + str(e) + '"); script cannot proceed')
      else:
        app.warn('\'' + command[0] + '\' not executed ("' + str(e) + '")')
        for p in _processes:
          p.terminate()
        _processes = [ ]
        break
    except (KeyboardInterrupt, SystemExit):
      import inspect, signal
      app._handler(signal.SIGINT, inspect.currentframe())

  return_stdout = ''
  return_stderr = ''
  error = False
  error_text = ''

  # Wait for all commands to complete
  try:

    # Switch how we monitor running processes / wait for them to complete
    #   depending on whether or not the user has specified -verbose or -debug option
    if app._verbosity > 1:
      for process in _processes:
        stderrdata = ''
        while True:
          # Have to read one character at a time: Waiting for a newline character using e.g. readline() will prevent MRtrix progressbars from appearing
          line = process.stderr.read(1).decode('utf-8')
          sys.stderr.write(line)
          sys.stderr.flush()
          stderrdata += line
          if not line and process.poll() is not None:
            break
        return_stderr += stderrdata
        if process.returncode:
          error = True
          error_text += stderrdata
    else:
      for process in _processes:
        process.wait()

  except (KeyboardInterrupt, SystemExit):
    import inspect, signal
    app._handler(signal.SIGINT, inspect.currentframe())

  # For any command stdout / stderr data that wasn't either passed to another command or
  #   printed to the terminal during execution, read it here.
  for index in range(len(cmdstack)):
    if tempfiles[index][0] is not None:
      tempfiles[index][0].flush()
      tempfiles[index][0].seek(0)
      stdout_text = tempfiles[index][0].read().decode('utf-8')
      return_stdout += stdout_text
      if _processes[index].returncode:
        error = True
        error_text += stdout_text
    if tempfiles[index][1] is not None:
      tempfiles[index][1].flush()
      tempfiles[index][1].seek(0)
      stderr_text = tempfiles[index][1].read().decode('utf-8')
      return_stderr += stderr_text
      if _processes[index].returncode:
        error = True
        error_text += stderr_text

  _processes = [ ]

  if (error):
    app._cleanup = False
    if exitOnError:
      caller = inspect.getframeinfo(inspect.stack()[1][0])
      app.console('')
      sys.stderr.write(os.path.basename(sys.argv[0]) + ': ' + app.colourError + '[ERROR] Command failed: ' + cmd + app.colourClear + app.colourDebug + ' (' + os.path.basename(caller.filename) + ':' + str(caller.lineno) + ')' + app.colourClear + '\n')
      sys.stderr.write(os.path.basename(sys.argv[0]) + ': ' + app.colourConsole + 'Output of failed command:' + app.colourClear + '\n')
      sys.stderr.write(error_text)
      sys.stderr.flush()
      if app._tempDir:
        with open(os.path.join(app._tempDir, 'error.txt'), 'w') as outfile:
          outfile.write(cmd + '\n\n' + error_text + '\n')
      app.complete()
      sys.exit(1)
    else:
      app.warn('Command failed: ' + cmd)

  # Only now do we append to the script log, since the command has completed successfully
  # Note: Writing the command as it was formed as the input to run.command():
  #   other flags may potentially change if this file is eventually used to resume the script
  if app._tempDir:
    with open(os.path.join(app._tempDir, 'log.txt'), 'a') as outfile:
      outfile.write(cmd + '\n')

  return (return_stdout, return_stderr)






def function(fn, *args):

  import os, sys
  from mrtrix3 import app

  fnstring = fn.__module__ + '.' + fn.__name__ + '(' + ', '.join(args) + ')'

  if app._lastFile:
    # Check to see if the last file produced in the previous script execution is
    #   intended to be produced by this command; if it is, this will be the last
    #   command that gets skipped by the -continue option
    # It's possible that the file might be defined in a '--option=XXX' style argument
    #  It's also possible that the filename in the command string has the file extension omitted
    for entry in args:
      if entry.startswith('--') and '=' in entry:
        totest = entry.split('=')[1]
      else:
        totest = entry
      filetotest = [ app._lastFile, os.path.splitext(app._lastFile)[0] ]
      if totest in filetotest:
        app.debug('Detected last file \'' + app._lastFile + '\' in function \'' + fnstring + '\'; this is the last run.command() / run.function() call that will be skipped')
        app._lastFile = ''
        break
    if app._verbosity:
      sys.stderr.write(app.colourExec + 'Skipping function:' + app.colourClear + ' ' + fnstring + '\n')
      sys.stderr.flush()
    return

  if app._verbosity:
    sys.stderr.write(app.colourExec + 'Function:' + app.colourClear + ' ' + fnstring + '\n')
    sys.stderr.flush()

  # Now we need to actually execute the requested function
  result = fn(*args)

  # Only now do we append to the script log, since the function has completed successfully
  if app._tempDir:
    with open(os.path.join(app._tempDir, 'log.txt'), 'a') as outfile:
      outfile.write(fnstring + '\n')

  return result







# When running on Windows, add the necessary '.exe' so that hopefully the correct
#   command is found by subprocess
def exeName(item):
  import os
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
  import os
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



# If the target executable is not a binary, but is actually a script, use the
#   shebang at the start of the file to alter the subprocess call
def _shebang(item):
  import os
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
