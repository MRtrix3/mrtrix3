_env = None



def command(cmd, exitOnError=True):

  import inspect, os, subprocess, sys
  from mrtrix3 import app, message, mrtrix

  global _env

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
    message.error('Malformed command \"' + cmd + '\": odd number of quotation marks')
  cmdsplit = [ ]
  if len(quotation_split) == 1:
    cmdsplit = cmd.split()
  else:
    for index, item in enumerate(quotation_split):
      if index%2:
        cmdsplit.append(item)
      else:
        cmdsplit.extend(item.split())

  if app.lastFile:
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
      filetotest = [ app.lastFile, os.path.splitext(app.lastFile)[0] ]
      if cmdtotest in filetotest:
        message.debug('Detected last file \'' + app.lastFile + '\' in command \'' + cmd + '\'; this is the last run.command() / run.function() call that will be skipped')
        app.lastFile = ''
        break
    if app.verbosity:
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
      is_mrtrix_exe = item in mrtrix.exe_list
      if is_mrtrix_exe:
        item = mrtrix.exeVersionMatch(item)
      next_is_exe = False
    if item == '|':
      if is_mrtrix_exe:
        if mrtrix.optionNThreads:
          new_cmdsplit.extend(mrtrix.optionNThreads.strip().split())
        if mrtrix.optionVerbosity:
          new_cmdsplit.append(mrtrix.optionVerbosity.strip())
      next_is_exe = True
    new_cmdsplit.append(item)
  if is_mrtrix_exe:
    if mrtrix.optionNThreads:
      new_cmdsplit.extend(mrtrix.optionNThreads.strip().split())
    if mrtrix.optionVerbosity:
      new_cmdsplit.append(mrtrix.optionVerbosity.strip())
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

  if app.verbosity:
    sys.stderr.write(message.colourExec + 'Command:' + message.colourClear + ' ' + cmd + '\n')
    sys.stderr.flush()

  message.debug('To execute: ' + str(cmdstack))

  # Execute all processes
  processes = [ ]
  for index, command in enumerate(cmdstack):
    if index > 0:
      proc_in = processes[index-1].stdout
    else:
      proc_in = None
    process = subprocess.Popen (command, stdin=proc_in, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=_env)
    processes.append(process)

  return_stdout = ''
  return_stderr = ''
  error = False
  error_text = ''

  # Wait for all commands to complete
  for process in processes:

    # Switch how we monitor running processes / wait for them to complete
    #   depending on whether or not the user has specified -verbose option
    if app.verbosity > 1:
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

  # Let all commands complete before grabbing stdout data; querying the stdout data
  #   immediately after command completion can intermittently prevent the data from
  #   getting to the following command (e.g. MRtrix piping)
  for process in processes:
    (stdoutdata, stderrdata) = process.communicate()
    stdoutdata = stdoutdata.decode('utf-8')
    stderrdata = stderrdata.decode('utf-8')
    return_stdout += stdoutdata + '\n'
    return_stderr += stderrdata + '\n'
    if process.returncode:
      error = True
      error_text += stdoutdata + stderrdata

  if (error):
    app.cleanup = False
    if exitOnError:
      caller = inspect.getframeinfo(inspect.stack()[1][0])
      message.console('')
      sys.stderr.write(os.path.basename(sys.argv[0]) + ': ' + message.colourError + '[ERROR] Command failed: ' + cmd + message.colourClear + message.colourDebug + ' (' + os.path.basename(caller.filename) + ':' + str(caller.lineno) + ')' + message.colourClear + '\n')
      sys.stderr.write(os.path.basename(sys.argv[0]) + ': ' + message.colourConsole + 'Output of failed command:' + message.colourClear + '\n')
      sys.stderr.write(error_text)
      sys.stderr.flush()
      if app.tempDir:
        with open(os.path.join(app.tempDir, 'error.txt'), 'w') as outfile:
          outfile.write(cmd + '\n\n' + error_text + '\n')
      app.complete()
      sys.exit(1)
    else:
      message.warn('Command failed: ' + cmd)

  # Only now do we append to the script log, since the command has completed successfully
  # Note: Writing the command as it was formed as the input to run.command():
  #   other flags may potentially change if this file is eventually used to resume the script
  if app.tempDir:
    with open(os.path.join(app.tempDir, 'log.txt'), 'a') as outfile:
      outfile.write(cmd + '\n')

  return (return_stdout, return_stderr)






def function(fn, *args):

  import os, sys
  from mrtrix3 import app, message

  fnstring = fn.__module__ + '.' + fn.__name__ + '(' + ', '.join(args) + ')'

  if app.lastFile:
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
      filetotest = [ app.lastFile, os.path.splitext(app.lastFile)[0] ]
      if totest in filetotest:
        message.debug('Detected last file \'' + app.lastFile + '\' in function \'' + fnstring + '\'; this is the last run.command() / run.function() call that will be skipped')
        app.lastFile = ''
        break
    if app.verbosity:
      sys.stderr.write(message.colourExec + 'Skipping function:' + message.colourClear + ' ' + fnstring + '\n')
      sys.stderr.flush()
    return

  if app.verbosity:
    sys.stderr.write(message.colourExec + 'Function:' + message.colourClear + ' ' + fnstring + '\n')
    sys.stderr.flush()

  # Now we need to actually execute the requested function
  result = fn(*args)

  # Only now do we append to the script log, since the function has completed successfully
  if app.tempDir:
    with open(os.path.join(app.tempDir, 'log.txt'), 'a') as outfile:
      outfile.write(fnstring + '\n')

  return result
