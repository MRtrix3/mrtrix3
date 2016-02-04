# Now have separate -quiet and -verbose options: 
# * -quiet prints nothing until script completes (unless warnings occur)
# * Default is to display command info, but no progress bars etc. (i.e. MRtrix still suppressed with -quiet)
# * -verbose prints all commands and all command outputs

args = ''
citationWarning = ''
cleanup = True
epilog = ''
lastFile = ''
mrtrixForce = ''
mrtrixQuiet = '-quiet'
mrtrixNThreads = ''
numArgs = 0
parser = ''
tempDir = ''
verbosity = 1
workingDir = ''

colourClear = ''
colourConsole = ''
colourError = ''
colourPrint = ''
colourWarn = ''


import argparse
class Parser(argparse.ArgumentParser):
  def error(self, message):
    import sys
    sys.stderr.write('\nError: %s\n\n' % message)
    self.print_help()
    sys.exit(2)



# Based on a list of names of commands used in the script, produce both the help page epilog and the citation warning to print at the commencement of the script
def initCitations(cmdlist):

  import lib.citations
  global epilog, citationWarning
  external_refs = False
  for name in cmdlist:
    entry = [item for item in lib.citations.list if item[0] == name][0]
    if entry:
      if entry[1]:
        external_refs = True
      # Construct string containing all relevant citations that will be fed to the argument parser epilog
      if not epilog:
        epilog = 'Relevant citations for tools / algorithms used in this script:\n\n'
      epilog += entry[0] + ':\n' + entry[2] + '\n\n'

  if epilog:
    citationWarning += 'Note that this script makes use of commands / algorithms that have relevant articles for citation'
    if external_refs:
      citationWarning += '; INCLUDING FROM EXTERNAL SOFTWARE PACKAGES'
    citationWarning += '. Please consult the help page (-help option) for more information.'




def initParser(desc):
  import argparse
  global epilog, parser
  parser = Parser(description=desc, epilog=epilog, add_help=False, formatter_class=argparse.RawDescriptionHelpFormatter)
  standard_options = parser.add_argument_group('standard options')
  standard_options.add_argument('-continue', nargs=2, dest='cont', metavar=('<TempDir>', '<LastFile>'), help='Continue the script from a previous execution; must provide the temporary directory path, and the name of the last successfully-generated file')
  standard_options.add_argument('-force', action='store_true', help='Force overwrite of output files if pre-existing')
  standard_options.add_argument('-help', action='store_true', help='Display help information for the script')
  standard_options.add_argument('-nocleanup', action='store_true', help='Do not delete temporary directory at script completion')
  standard_options.add_argument('-nthreads', metavar='number', help='Use this number of threads in MRtrix multi-threaded applications (0 disables multi-threading)')
  standard_options.add_argument('-tempdir', metavar='/path/to/tmp/', help='Manually specify the path in which to generate the temporary directory')
  verbosity_group = standard_options.add_mutually_exclusive_group()
  verbosity_group.add_argument('-quiet',   action='store_true', help='Suppress all console output during script execution')
  verbosity_group.add_argument('-verbose', action='store_true', help='Display additional information for every command invoked')



def initialise():
  import argparse, os, random, string, sys
  from lib.printMessage          import printMessage
  from lib.readMRtrixConfSetting import readMRtrixConfSetting
  global args, cleanup, lastFile, mrtrixNThreads, mrtrixQuiet, tempDir, verbosity, workingDir
  global colourClear, colourConsole, colourError, colourPrint, colourWarn
  workingDir = os.getcwd()
  args = parser.parse_args()
  if args.help:
    parser.print_help()
    sys.exit(0)

  use_colour = readMRtrixConfSetting('TerminalColor')
  if use_colour:
    use_colour = use_colour.lower() in ('yes', 'true', '1')
  else:
    use_colour = not sys.platform.startswith('win')
  if use_colour:
    colourClear = '\033[0m'
    colourConsole = '\033[03;34m'
    colourError = '\033[01;31m'
    colourPrint = '\033[03;32m'
    colourWarn = '\033[00;31m'

  if citationWarning:
    printMessage('')
    printMessage(citationWarning)
    printMessage('')

  if args.nocleanup:
    cleanup = False
  if args.nthreads:
    mrtrixNThreads = '-nthreads ' + args.nthreads
  if args.quiet:
    verbosity = 0
    mrtrixQuiet = '-quiet'
  if args.verbose:
    verbosity = 2
    mrtrixQuiet = ''
  if args.cont:
    tempDir = os.path.abspath(args.cont[0])
    lastFile = args.cont[1]
  else:
    if args.tempdir:
      dir_path = os.path.abspath(args.tempdir)
    else:
      dir_path = readMRtrixConfSetting('TmpFileDir')
      if not dir_path:
        if os.name == 'posix':
          dir_path = '/tmp'
        else:
          dir_path = '.'
    prefix = readMRtrixConfSetting('TmpFilePrefix')
    if not prefix:
      prefix = os.path.basename(sys.argv[0]) + '-tmp-'
    tempDir = dir_path
    while os.path.isdir(tempDir):
      random_string = ''.join(random.choice(string.ascii_uppercase + string.digits) for x in range(6))
      tempDir = os.path.join(dir_path, prefix + random_string) + os.sep
    os.makedirs(tempDir)
    printMessage('Generated temporary directory: ' + tempDir)
    with open(os.path.join(tempDir, 'cwd.txt'), 'w') as outfile:
      outfile.write(workingDir + '\n')
    with open(os.path.join(tempDir, 'command.txt'), 'w') as outfile:
      outfile.write(' '.join(sys.argv) + '\n')



def checkOutputFile(path):
  import os
  from lib.errorMessage import errorMessage
  from lib.warnMessage  import warnMessage
  global args, mrtrixForce
  if not path:
    return
  if os.path.exists(path):
    if args.force:
      warnMessage('Output file already exists; will be overwritten at script completion')
      mrtrixForce = ' -force'
    else:
      errorMessage('Output file already exists (use -force to override)')
      sys.exit(1)



def gotoTempDir():
  import os
  from lib.printMessage import printMessage
  if verbosity:
    printMessage('Changing to temporary directory (' + tempDir + ')')
  os.chdir(tempDir)



def complete():
  import os, shutil, sys
  from lib.printMessage import printMessage
  printMessage('Changing back to original directory (' + workingDir + ')')
  os.chdir(workingDir)
  if cleanup:
    printMessage('Deleting temporary directory ' + tempDir)
    shutil.rmtree(tempDir)
  else:
    # This needs to be printed even if the -quiet option is used
    sys.stdout.write(os.path.basename(sys.argv[0]) + ': ' + colourPrint + 'Contents of temporary directory kept, location: ' + tempDir + colourClear + '\n')
    sys.stdout.flush()

