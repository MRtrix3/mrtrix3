mrtrix_bin_list = [ ]

def runCommand(cmd, exitOnError=True):

  import inspect, lib.app, os, subprocess, sys
  from lib.debugMessage import debugMessage
  from lib.errorMessage import errorMessage
  from lib.isWindows    import isWindows
  from lib.printMessage import printMessage
  from lib.warnMessage  import warnMessage
  import distutils
  from distutils.spawn import find_executable
  global mrtrix_bin_list
  global mrtrix_bin_path

  if not mrtrix_bin_list:
    mrtrix_bin_path = os.path.abspath(os.path.join(os.path.abspath(os.path.dirname(os.path.realpath(__file__))), os.pardir, os.pardir, 'release', 'bin'));
    # On Windows, strip the .exe's
    mrtrix_bin_list = [ os.path.splitext(name)[0] for name in os.listdir(mrtrix_bin_path) ]

  if lib.app.lastFile:
    # Check to see if the last file produced is produced by this command;
    #   if it is, this will be the last called command that gets skipped
    if lib.app.lastFile in cmd:
      lib.app.lastFile = ''
    if lib.app.verbosity:
      sys.stderr.write(lib.app.colourConsole + 'Skipping command:' + lib.app.colourClear + ' ' + cmd + '\n')
    return

  # Vectorise the command string, preserving anything encased within quotation marks
  # TODO Use shlex.split()?
  quotation_split = cmd.split('\"')
  if not len(quotation_split)%2:
    errorMessage('Malformed command \"' + cmd + '\": odd number of quotation marks')
  cmdsplit = [ ]
  if len(quotation_split) == 1:
    cmdsplit = cmd.split()
  else:
    for index, item in enumerate(quotation_split):
      if index%2:
        cmdsplit.append(item)
      else:
        cmdsplit.extend(item.split())

  # For any MRtrix commands, need to insert the nthreads and quiet calls
  new_cmdsplit = [ ]
  is_mrtrix_binary = False
  next_is_binary = True
  for item in cmdsplit:
    if next_is_binary:
      is_mrtrix_binary = item in mrtrix_bin_list
      # Make sure we're not accidentally running an MRtrix command from a different installation to the script
      if is_mrtrix_binary:
        binary_sys = find_executable(item)
        binary_manual = os.path.join(mrtrix_bin_path, item)
        if (isWindows()):
          binary_manual = binary_manual + '.exe'
        use_manual_binary_path = not binary_sys
        if not use_manual_binary_path:
          # os.path.samefile() not supported on all platforms / Python versions
          if hasattr(os.path, 'samefile'):
            use_manual_binary_path = not os.path.samefile(binary_sys, binary_manual)
          else:
            # Hack equivalent of samefile(); not perfect, but should be adequate for use here
            use_manual_binary_path = not os.path.normcase(os.path.normpath(binary_sys)) == os.path.normcase(os.path.normpath(binary_manual))
        if use_manual_binary_path:
          item = binary_manual
      next_is_binary = False
    if item == '|':
      if is_mrtrix_binary:
        if lib.app.mrtrixNThreads:
          new_cmdsplit.extend(lib.app.mrtrixNThreads.strip().split())
        if lib.app.mrtrixVerbosity:
          new_cmdsplit.append(lib.app.mrtrixVerbosity.strip())
      next_is_binary = True
    new_cmdsplit.append(item)
  if is_mrtrix_binary:
    if lib.app.mrtrixNThreads:
      new_cmdsplit.extend(lib.app.mrtrixNThreads.strip().split())
    if lib.app.mrtrixVerbosity:
      new_cmdsplit.append(lib.app.mrtrixVerbosity.strip())
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
    sys.stderr.write(lib.app.colourConsole + 'Command:' + lib.app.colourClear + ' ' + cmd + '\n')

  debugMessage('To execute: ' + str(cmdstack))

  # Execute all processes
  processes = [ ]
  for index, command in enumerate(cmdstack):
    if index > 0:
      proc_in = processes[index-1].stdout
    else:
      proc_in = None
    process = subprocess.Popen (command, stdin=proc_in, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
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
      printMessage('')
      sys.stderr.write(os.path.basename(sys.argv[0]) + ': ' + lib.app.colourError + '[ERROR] Command failed: ' + cmd + lib.app.colourClear + lib.app.colourDebug + ' (' + os.path.basename(caller.filename) + ':' + str(caller.lineno) + ')' + lib.app.colourClear + '\n')
      sys.stderr.write(os.path.basename(sys.argv[0]) + ': ' + lib.app.colourPrint + 'Output of failed command:' + lib.app.colourClear + '\n')
      sys.stderr.write(error_text)
      if lib.app.tempDir:
        with open(os.path.join(lib.app.tempDir, 'error.txt'), 'w') as outfile:
          outfile.write(cmd + '\n\n' + error_text + '\n')
      lib.app.complete()
      exit(1)
    else:
      warnMessage('Command failed: ' + cmd)

  # Only now do we append to the script log, since the command has completed successfully
  # Note: Writing the command as it was formed as the input to runCommand():
  #   other flags may potentially change if this file is eventually used to resume the script
  if lib.app.tempDir:
    with open(os.path.join(lib.app.tempDir, 'log.txt'), 'a') as outfile:
      outfile.write(cmd + '\n')

  return (return_stdout, return_stderr)

