mrtrix_bin_list = [ ]

def runCommand(cmd, exitOnError=True):

  import lib.app, os, sys
  from lib.errorMessage import errorMessage
  from lib.warnMessage  import warnMessage
  import distutils
  from distutils.spawn import find_executable
  global mrtrix_bin_list
  global mrtrix_bin_path
  
  if not mrtrix_bin_list:
    mrtrix_bin_path = os.path.join(os.path.abspath(os.path.dirname(os.path.realpath(__file__))), os.pardir, os.pardir, 'release', 'bin');
    mrtrix_bin_list = os.listdir(mrtrix_bin_path)
    
  if lib.app.lastFile:
    # Check to see if the last file produced is produced by this command;
    #   if it is, this will be the last called command that gets skipped
    if lib.app.lastFile in cmd:
      lib.app.lastFile = ''
    if lib.app.verbosity:
      sys.stdout.write('Skipping command: ' + cmd + '\n')
      sys.stdout.flush()
    return

  binary_name = cmd.split()[0]

  # Automatically add the relevant flags to any mrtrix command calls, including filling them in around the pipes
  if binary_name in mrtrix_bin_list:
    cmdsplit = cmd.split()
    binary_in_path = find_executable(binary_name)
    if not binary_in_path or not os.path.samefile(binary_in_path, os.path.join(mrtrix_bin_path, binary_name)):
      cmdsplit[0] =  os.path.join(mrtrix_bin_path, binary_name)
    for index, item in enumerate(cmdsplit):
      if item == '|':
        if lib.app.mrtrixNThreads:
          cmdsplit[index] = lib.app.mrtrixNThreads + ' |'
        if lib.app.mrtrixQuiet:
          cmdsplit[index] = lib.app.mrtrixQuiet + ' ' + cmdsplit[index]
    cmdsplit.append(lib.app.mrtrixNThreads)
    cmdsplit.append(lib.app.mrtrixQuiet)
    cmd = ' '.join(cmdsplit)
    
  if lib.app.verbosity:
    sys.stdout.write(lib.app.colourConsole + 'Command:' + lib.app.colourClear + ' ' + cmd + '\n')
    sys.stdout.flush()

  if (os.system(cmd)):
    if exitOnError:
      errorMessage('Command failed: ' + cmd)
    else:
      warnMessage('Command failed: ' + cmd)

