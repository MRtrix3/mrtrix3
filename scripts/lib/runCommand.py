_env = None

def runCommand(cmd, exitOnError=True):

  import inspect, os, subprocess, sys
  import lib.app
  import lib.message
  import lib.mrtrix

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
    lib.message.error('Malformed command \"' + cmd + '\": odd number of quotation marks')
  cmdsplit = [ ]
  if len(quotation_split) == 1:
    cmdsplit = cmd.split()
  else:
    for index, item in enumerate(quotation_split):
      if index%2:
        cmdsplit.append(item)
      else:
        cmdsplit.extend(item.split())

  if lib.app.lastFile:
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
      filetotest = [ lib.app.lastFile, os.path.splitext(lib.app.lastFile)[0] ]
      if cmdtotest in filetotest:
        lib.message.debug('Detected last file \'' + lib.app.lastFile + '\' in command \'' + cmd + '\'; this is the last runCommand() / runFunction() call that will be skipped')
        lib.app.lastFile = ''
        break
    if lib.app.verbosity:
      sys.stderr.write(lib.app.colourExec + 'Skipping command:' + lib.app.colourClear + ' ' + cmd + '\n')
      sys.stderr.flush()
    return

  # For any MRtrix commands, need to insert the nthreads and quiet calls
  # Also make sure that the appropriate version of that MRtrix command will be invoked
  new_cmdsplit = [ ]
  is_mrtrix_exe = False
  next_is_exe = True
  for item in cmdsplit:
    if next_is_exe:
      is_mrtrix_exe = item in lib.mrtrix.exe_list
      if is_mrtrix_exe:
        item = lib.mrtrix.exeVersionMatch(item)
      next_is_exe = False
    if item == '|':
      if is_mrtrix_exe:
        if lib.mrtrix.optionNThreads:
          new_cmdsplit.extend(lib.mrtrix.optionNThreads.strip().split())
        if lib.mrtrix.optionVerbosity:
          new_cmdsplit.append(lib.mrtrix.optionVerbosity.strip())
      next_is_exe = True
    new_cmdsplit.append(item)
  if is_mrtrix_exe:
    if lib.mrtrix.optionNThreads:
      new_cmdsplit.extend(lib.mrtrix.optionNThreads.strip().split())
    if lib.mrtrix.optionVerbosity:
      new_cmdsplit.append(lib.mrtrix.optionVerbosity.strip())
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

  if lib.app.verbosity:
    sys.stderr.write(lib.message.colourExec + 'Command:' + lib.message.colourClear + ' ' + cmd + '\n')
    sys.stderr.flush()

  lib.message.debug('To execute: ' + str(cmdstack))

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
    if lib.app.verbosity > 1:
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
    lib.app.cleanup = False
    if exitOnError:
      caller = inspect.getframeinfo(inspect.stack()[1][0])
      lib.message.console('')
      sys.stderr.write(os.path.basename(sys.argv[0]) + ': ' + lib.message.colourError + '[ERROR] Command failed: ' + cmd + lib.message.colourClear + lib.message.colourDebug + ' (' + os.path.basename(caller.filename) + ':' + str(caller.lineno) + ')' + lib.message.colourClear + '\n')
      sys.stderr.write(os.path.basename(sys.argv[0]) + ': ' + lib.message.colourPrint + 'Output of failed command:' + lib.message.colourClear + '\n')
      sys.stderr.write(error_text)
      sys.stderr.flush()
      if lib.app.tempDir:
        with open(os.path.join(lib.app.tempDir, 'error.txt'), 'w') as outfile:
          outfile.write(cmd + '\n\n' + error_text + '\n')
      lib.app.complete()
      sys.exit(1)
    else:
      lib.message.warn('Command failed: ' + cmd)

  # Only now do we append to the script log, since the command has completed successfully
  # Note: Writing the command as it was formed as the input to runCommand():
  #   other flags may potentially change if this file is eventually used to resume the script
  if lib.app.tempDir:
    with open(os.path.join(lib.app.tempDir, 'log.txt'), 'a') as outfile:
      outfile.write(cmd + '\n')

  return (return_stdout, return_stderr)

