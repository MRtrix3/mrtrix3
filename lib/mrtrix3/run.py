_env = None

import os
_mrtrix_bin_path = os.path.join(os.path.abspath(os.path.dirname(os.path.abspath(__file__))), os.pardir, os.pardir, 'bin')
# Remember to remove the '.exe' from Windows binary executables
_mrtrix_exe_list = [ os.path.splitext(name)[0] for name in os.listdir(_mrtrix_bin_path) ]

_processes = [ ]



def command(cmd, exitOnError=True):

  import inspect, os, subprocess, sys
  from mrtrix3 import app

  global _env, _mrtrix_exe_list

  if not _env:
    _env = os.environ.copy()
    # Prevent _any_ SGE-compatible commands from running in SGE mode;
    #   this is a simpler solution than trying to monitor a queued job
    # If one day somebody wants to use this scripting framework to execute queued commands,
    #   an alternative solution will need to be found
    if os.environ.get('SGE_ROOT'):
      del _env['SGE_ROOT']

  # Vectorise the command string, preserving anything encased within quotation marks
  # TODO Use shlex.split()?
  quotation_split = cmd.split('\"')
  if not len(quotation_split)%2:
    app.error('Malformed command \"' + cmd + '\": odd number of quotation marks')
  cmdsplit = [ ]
  if len(quotation_split) == 1:
    cmdsplit = cmd.split()
  else:
    for index, item in enumerate(quotation_split):
      if index%2:
        cmdsplit.append(item)
      else:
        cmdsplit.extend(item.split())

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
      filetotest = [ app._lastFile, os.path.splitext(app.lastFile)[0] ]
      if cmdtotest in filetotest:
        app.debug('Detected last file \'' + app._lastFile + '\' in command \'' + cmd + '\'; this is the last run.command() / run.function() call that will be skipped')
        app.lastFile = ''
        break
    if app._verbosity:
      sys.stderr.write(app.colourExec + 'Skipping command:' + app.colourClear + ' ' + cmd + '\n')
      sys.stderr.flush()
    return

  # For any MRtrix commands, need to insert the nthreads and quiet calls
  # Also make sure that the appropriate version of that MRtrix command will be invoked
  new_cmdsplit = [ ]
  is_mrtrix_exe = False
  next_is_exe = True
  for item in cmdsplit:
    if next_is_exe:
      is_mrtrix_exe = item in _mrtrix_exe_list
      if is_mrtrix_exe:
        item = versionMatch(item)
      next_is_exe = False
    if item == '|':
      if is_mrtrix_exe:
        if app._nthreads is not None:
          new_cmdsplit.extend( [ '-nthreads', str(app._nthreads) ] )
        # Get MRtrix3 binaries to output additional INFO-level information if running in debug mode
        if app._verbosity == 3:
          new_cmdsplit.append('-info')
      next_is_exe = True
    new_cmdsplit.append(item)
  if is_mrtrix_exe:
    if app._nthreads is not None:
      new_cmdsplit.extend( [ '-nthreads', str(app._nthreads) ] )
    if app._verbosity == 3:
      new_cmdsplit.append('-info')
  cmdsplit = new_cmdsplit

  # If the piping symbol appears anywhere, we need to split this into multiple commands and execute them separately
  # If no piping symbols, the entire command should just appear as a single row in cmdstack
  cmdstack = [ ]
  prev = 0
  for index, item in enumerate(cmdsplit):
    if item == '|':
      cmdstack.append(cmdsplit[prev:index])
      prev = index + 1
  cmdstack.append(cmdsplit[prev:])

  if app._verbosity:
    sys.stderr.write(app.colourExec + 'Command:' + app.colourClear + '  ' + cmd + '\n')
    sys.stderr.flush()

  app.debug('To execute: ' + str(cmdstack))

  # Execute all processes
  _processes = [ ]
  for index, command in enumerate(cmdstack):
    if index > 0:
      proc_in = _processes[index-1].stdout
    else:
      proc_in = None
    try:
      process = subprocess.Popen (command, stdin=proc_in, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=_env)
      _processes.append(process)
    except FileNotFoundError:
      if exitOnError:
        app.error('\'' + command[0] + '\' not found in PATH; script cannot proceed')
      else:
        app.warn('\'' + command[0] + '\' not found in PATH; not executed')
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
    for process in _processes:

      # Switch how we monitor running processes / wait for them to complete
      #   depending on whether or not the user has specified -verbose option
      if app._verbosity > 1:
        stderrdata = ''
        while True:
          # Have to read one character at a time: Waiting for a newline character using e.g. readline() will prevent MRtrix progressbars from appearing
          line = process.stderr.read(1).decode('utf-8')
          sys.stderr.write(line)
          sys.stderr.flush()
          stderrdata += line
          if not line and process.poll() != None:
            break
      else:
        process.wait()
  except (KeyboardInterrupt, SystemExit):
    import inspect, signal
    app._handler(signal.SIGINT, inspect.currentframe())

  # Let all commands complete before grabbing stdout data; querying the stdout data
  #   immediately after command completion can intermittently prevent the data from
  #   getting to the following command (e.g. MRtrix piping)
  for process in _processes:
    (stdoutdata, stderrdata) = process.communicate()
    stdoutdata = stdoutdata.decode('utf-8')
    stderrdata = stderrdata.decode('utf-8')
    return_stdout += stdoutdata + '\n'
    return_stderr += stderrdata + '\n'
    if process.returncode:
      error = True
      error_text += stdoutdata + stderrdata
    process = None
  _processes = [ ]

  if (error):
    app.cleanup = False
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
        app.lastFile = ''
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







# Make sure we're not accidentally running an MRtrix executable on the system that
#   belongs to a different version of MRtrix3 to the script library currently being used
def versionMatch(item):
  import distutils, os, sys
  from distutils.spawn import find_executable
  from mrtrix3 import app
  global _mrtrix_bin_path, _mrtrix_exe_list

  if not item in _mrtrix_exe_list:
    app.debug('Command ' + item + ' not found in MRtrix3 bin/ directory')
    return item

  exe_path_sys = find_executable(item)
  exe_path_manual = os.path.join(_mrtrix_bin_path, item)
  if not os.path.isfile(exe_path_manual):
    exe_path_manual = exe_path_manual + '.exe'
    if not os.path.isfile(exe_path_manual):
      exe_path_manual = ''

  # Always use the manual path if the item isn't found in the system path
  use_manual_exe_path = not exe_path_sys
  if not use_manual_exe_path:
    # os.path.samefile() not supported on all platforms / Python versions
    if hasattr(os.path, 'samefile'):
      use_manual_exe_path = not os.path.samefile(exe_path_sys, exe_path_manual)
    else:
      # Hack equivalent of samefile(); not perfect, but should be adequate for use here
      use_manual_exe_path = not os.path.normcase(os.path.normpath(exe_path_sys)) == os.path.normcase(os.path.normpath(exe_path_manual))

  if use_manual_exe_path:
    app.debug('Forcing version match for ' + item + ': ' + exe_path_manual)
    return exe_path_manual
  else:
    app.debug('System version of ' + item + ' matches MRtrix3 version')
    return item



