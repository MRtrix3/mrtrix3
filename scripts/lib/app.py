# Now have separate -quiet and -verbose options: 
# * -quiet prints nothing until script completes (unless warnings occur)
# * Default is to display command info, but no progress bars etc. (i.e. MRtrix still suppressed with -quiet)
# * -verbose prints all commands and all command outputs

args = ''
author = 'No author specified'
citationWarning = ''
cleanup = True
lastFile = ''
mrtrixQuiet = '-quiet'
mrtrixNThreads = ''
numArgs = 0
parser = ''
refList = ''
standardOptions = ''
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
  global refList, citationWarning
  max_level = 0
  for name in cmdlist:
    entry = [item for item in lib.citations.list if item[0] == name][0]
    if entry:
      max_level = max(max_level, entry[1]);
      # Construct string containing all relevant citations that will be fed to the argument parser epilog
      if refList:
        refList += '\n'
      refList += entry[0] + ': ' + entry[2] + '\n'

  if max_level:
    citationWarning += 'Note that this script makes use of commands / algorithms that have relevant articles for citation'
    if max_level > 1:
      citationWarning += '; INCLUDING FROM EXTERNAL SOFTWARE PACKAGES'
    citationWarning += '. Please consult the help page (-help option) for more information.'




def initParser(desc):
  import argparse
  global parser, refList, standardOptions
  epilog = ''
  if refList:
    epilog = 'Relevant citations for tools / algorithms used in this script:\n\n' + refList + '\n'
  epilog += 'Author:\n' + author + '\n\nCopyright (C) 2008 Brain Research Institute, Melbourne, Australia. This is free software; see the source for copying conditions. There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n'
  parser = Parser(description=desc, epilog=epilog, formatter_class=argparse.RawDescriptionHelpFormatter)
  standardOptions = parser.add_argument_group('standard options')
  standardOptions.add_argument('-continue', nargs=2, dest='cont', metavar=('<TempDir>', '<LastFile>'), help='Continue the script from a previous execution; must provide the temporary directory path, and the name of the last successfully-generated file')
  standardOptions.add_argument('-nocleanup', action='store_true', help='Do not delete temporary directory at script completion')
  standardOptions.add_argument('-nthreads', metavar='number', help='Use this number of threads in MRtrix multi-threaded applications (0 disables multi-threading)')
  standardOptions.add_argument('-tempdir', metavar='/path/to/tmp/', help='Manually specify the path in which to generate the temporary directory')
  verbosity_group = standardOptions.add_mutually_exclusive_group()
  verbosity_group.add_argument('-quiet',   action='store_true', help='Suppress all console output during script execution')
  verbosity_group.add_argument('-verbose', action='store_true', help='Display additional information for every command invoked')



def initialise():
  import argparse, os, random, string, sys
  from lib.printMessage          import printMessage
  from lib.printUsageMarkdown    import printUsageMarkdown
  from lib.readMRtrixConfSetting import readMRtrixConfSetting
  global args, author, cleanup, lastFile, mrtrixNThreads, mrtrixQuiet, standardOptions, parser, refList, tempDir, verbosity, workingDir
  global colourClear, colourConsole, colourError, colourPrint, colourWarn
  
  if len(sys.argv) == 2 and sys.argv[1] == '__print_usage_markdown__':
    printUsageMarkdown(parser, standardOptions, refList, author)
    exit(0)
  
  workingDir = os.getcwd()
  args = parser.parse_args()

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




def gotoTempDir():
  import os
  from lib.printMessage import printMessage
  if verbosity:
    printMessage('Changing to temporary directory (' + tempDir + ')')
  os.chdir(tempDir)



def moveFileToDest(local_path, destination):
  import os, shutil
  from lib.printMessage import printMessage
  if destination[0] != '/':
    destination = os.path.abspath(os.path.join(workingDir, destination))
  printMessage('Moving output file from temporary directory to user specified location')
  shutil.move(local_path, destination)



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

