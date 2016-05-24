mrtrix_bin_list = [ ]

def runCommand(cmd, exitOnError=True):

  import lib.app, os, subprocess, sys
  from lib.errorMessage import errorMessage
  from lib.isWindows    import isWindows
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
      sys.stdout.write('Skipping command: ' + cmd + '\n')
      sys.stdout.flush()
    return

  # Vectorise the command string, preserving anything encased within quotation marks
  # This will eventually allow the use of subprocess rather than os.system()
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
      binary_sys = find_executable(item)
      binary_manual = os.path.join(mrtrix_bin_path, item)
      if (isWindows()):
        binary_manual = binary_manual + '.exe'
      if not binary_sys or not os.path.samefile(binary_sys, binary_manual):
        item = binary_manual
      next_is_binary = False
    if item == '|':
      if is_mrtrix_binary:
        if lib.app.mrtrixNThreads:
          new_cmdsplit.extend(lib.app.mrtrixNThreads.strip().split())
        if lib.app.mrtrixQuiet:
          new_cmdsplit.append(lib.app.mrtrixQuiet.strip())
      next_is_binary = True
    new_cmdsplit.append(item)
  if is_mrtrix_binary:
    if lib.app.mrtrixNThreads:
      new_cmdsplit.extend(lib.app.mrtrixNThreads.strip().split())
    if lib.app.mrtrixQuiet:
      new_cmdsplit.append(lib.app.mrtrixQuiet.strip())
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
    sys.stdout.write(lib.app.colourConsole + 'Command:' + lib.app.colourClear + ' ' + cmd + '\n')
    sys.stdout.flush()

  error = False
  if len(cmdstack) == 1:
    error = subprocess.call (cmdstack[0], stdin=None, stdout=None, stderr=None)
  else:
    processes = [ ]
    for index, command in enumerate(cmdstack):
      if index > 0:
        proc_in = processes[index-1].stdout
      else:
        proc_in = None
      if index < len(cmdstack)-1:
        proc_out = subprocess.PIPE
      else:
        proc_out = None
      process = subprocess.Popen (command, stdin=proc_in, stdout=proc_out, stderr=None)
      processes.append(process)

    # Wait for all commands to complete
    for process in processes:
      process.wait()
      if process.returncode:
        error = True

  if (error):
    if exitOnError:
      errorMessage('Command failed: ' + cmd)
    else:
      warnMessage('Command failed: ' + cmd)

