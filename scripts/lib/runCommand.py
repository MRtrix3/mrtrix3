mrtrix_bin_list = [ ]

def runCommand(cmd):

  import app, os, sys
  global mrtrix_bin_list
  
  if not mrtrix_bin_list:
    mrtrix_bin_path = os.path.join(os.path.abspath(os.path.dirname(os.path.realpath(sys.argv[0]))), os.pardir, 'bin');
    mrtrix_bin_list = [ f for f in os.listdir(mrtrix_bin_path) if not "__" in f ]
    
  binary_name = cmd.split()[0]
    
  # Automatically add the -quiet flags to any mrtrix command calls, including filling them in around the pipes
  if binary_name in mrtrix_bin_list:
    cmdsplit = cmd.split()
    for index, item in enumerate(cmdsplit):
      if item == '|':
        cmdsplit[index] = app.mrtrixQuiet + ' |'
        index += 1
    cmdsplit.append(app.mrtrixQuiet)
    cmd = ' '.join(cmdsplit)
    
  if app.verbosity:
  	sys.stdout.write('Command: ' + cmd + '\n')
  	sys.stdout.flush()

  if (os.system(cmd)):
    sys.stderr.write('Command failed: ' + cmd + '\n')
    exit(1)

