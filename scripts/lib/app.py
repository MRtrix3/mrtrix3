
args = ''
author = ''
citationList = []
cleanup = True
copyright = '''Copyright (c) 2008-2016 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/

MRtrix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see www.mrtrix.org'''
externalCitations = False
lastFile = ''
mrtrixForce = ''
mrtrixVerbosity = ' -quiet'
mrtrixNThreads = ''
parser = ''
tempDir = ''
verbosity = 1
workingDir = ''

clearLine = ''
colourClear = ''
colourConsole = ''
colourDebug = ''
colourError = ''
colourPrint = ''
colourWarn = ''



def addCitation(condition, reference, is_external):
  global citationList, externalCitations
  citationList.append( (condition, reference) )
  if is_external:
    externalCitations = True



def initialise():
  import os, signal, sys
  import lib.signalHandler
  from lib.errorMessage          import errorMessage
  from lib.printMessage          import printMessage
  from lib.readMRtrixConfSetting import readMRtrixConfSetting
  global args, citationList, cleanup, externalCitations, lastFile, mrtrixNThreads, mrtrixVerbosity, parser, tempDir, verbosity, workingDir
  global colourClear, colourConsole, colourDebug, colourError, colourPrint, colourWarn

  if not parser:
    errorMessage('Script error: Command-line parser must be initialised before app')

  if len(sys.argv) == 1:
    parser.print_help()
    sys.exit(0)

  if sys.argv[-1] == '__print_usage_markdown__':
    parser.printUsageMarkdown()
    exit(0)

  if sys.argv[-1] == '__print_usage_rst__':
    parser.printUsageRst()
    exit(0)

  workingDir = os.getcwd()

  args = parser.parse_args()
  if args.help:
    parser.print_help()
    sys.exit(0)

  use_colour = readMRtrixConfSetting('TerminalColor')
  if use_colour:
    use_colour = use_colour.lower() in ('yes', 'true', '1')
  else:
    # Windows now also gets coloured text terminal support, so make this the default
    use_colour = True
  if use_colour:
    clearLine = '\033[0K'
    colourClear = '\033[0m'
    colourConsole = '\033[03;36m'
    colourDebug = '\033[03;34m'
    colourError = '\033[01;31m'
    colourPrint = '\033[03;32m'
    colourWarn = '\033[00;31m'

  if args.nocleanup:
    cleanup = False
  if args.nthreads:
    mrtrixNThreads = ' -nthreads ' + args.nthreads
  if args.quiet:
    verbosity = 0
    mrtrixVerbosity = ' -quiet'
  elif args.verbose:
    verbosity = 2
    mrtrixVerbosity = ''
  elif args.debug:
    verbosity = 3
    mrtrixVerbosity = ' -info'

  if citationList:
    printMessage('')
    citation_warning = 'Note that this script makes use of commands / algorithms that have relevant articles for citation'
    if externalCitations:
      citation_warning += '; INCLUDING FROM EXTERNAL SOFTWARE PACKAGES'
    citation_warning += '. Please consult the help page (-help option) for more information.'
    printMessage(citation_warning)
    printMessage('')

  lib.signalHandler.initialise()

  if args.cont:
    tempDir = os.path.abspath(args.cont[0])
    lastFile = args.cont[1]



def checkOutputFile(path):
  import os
  from lib.errorMessage import errorMessage
  from lib.warnMessage  import warnMessage
  global args, mrtrixForce
  if not path:
    return
  if os.path.exists(path):
    type = ''
    if os.path.isfile(path):
      type = ' file'
    elif os.path.isdir(path):
      type = ' directory'
    if args.force:
      warnMessage('Output' + type + ' ' + os.path.basename(path) + ' already exists; will be overwritten at script completion')
      mrtrixForce = ' -force'
    else:
      errorMessage('Output' + type + ' ' + path + ' already exists (use -force to override)')
      sys.exit(1)



def makeTempDir():
  import os, random, string, sys
  from lib.errorMessage          import errorMessage
  from lib.printMessage          import printMessage
  from lib.readMRtrixConfSetting import readMRtrixConfSetting
  global args, tempDir, workingDir
  if args.cont:
    printMessage('Skipping temporary directory creation due to use of -continue option')
    return
  if tempDir:
    errorMessage('Script error: Cannot use multiple temporary directories')
  if args.tempdir:
    dir_path = os.path.abspath(args.tempdir)
  else:
    dir_path = readMRtrixConfSetting('TmpFileDir')
    if not dir_path:
      if os.name == 'posix':
        dir_path = '/tmp'
      else:
        dir_path = workingDir
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
  open(os.path.join(tempDir, 'log.txt'), 'w').close()



def gotoTempDir():
  import os
  from lib.errorMessage import errorMessage
  from lib.printMessage import printMessage
  global tempDir
  if not tempDir:
    errorMessage('Script error: No temporary directory location set')
  if verbosity:
    printMessage('Changing to temporary directory (' + tempDir + ')')
  os.chdir(tempDir)



def complete():
  import os, shutil, sys
  from lib.printMessage import printMessage
  global colourClear, colourPrint, colourWarn, tempDir, workingDir
  printMessage('Changing back to original directory (' + workingDir + ')')
  os.chdir(workingDir)
  if cleanup and tempDir:
    printMessage('Deleting temporary directory ' + tempDir)
    shutil.rmtree(tempDir)
  elif tempDir:
    # This needs to be printed even if the -quiet option is used
    if os.path.isfile(os.path.join(tempDir, 'error.txt')):
      with open(os.path.join(tempDir, 'error.txt'), 'r') as errortext:
        sys.stderr.write(os.path.basename(sys.argv[0]) + ': ' + colourWarn + 'Script failed while executing the command: ' + errortext.readline().rstrip() + colourClear + '\n')
      sys.stderr.write(os.path.basename(sys.argv[0]) + ': ' + colourWarn + 'For debugging, inspect contents of temporary directory: ' + tempDir + colourClear + '\n')
    else:
      sys.stderr.write(os.path.basename(sys.argv[0]) + ': ' + colourPrint + 'Contents of temporary directory kept, location: ' + tempDir + colourClear + '\n')
    sys.stderr.flush()



def make_dir(dir):
  import os
  from lib.debugMessage import debugMessage
  if not os.path.exists(dir):
    os.makedirs(dir)
    debugMessage('Created directory ' + dir)
  else:
    debugMessage('Directory ' + dir + ' already exists')



# determines the common postfix for a list of filenames (including the file extension)
def getCommonPostfix(inputFiles):
  from lib.debugMessage import debugMessage
  first = inputFiles[0]
  cursor = 0
  found = False
  common = ''
  for i in reversed(first):
    if found == False:
      for j in inputFiles:
        if j[len(j)-cursor-1] != first[len(first)-cursor-1]:
          found = True
          break
      if found == False:
        common = first[len(first)-cursor-1] + common
      cursor += 1
  debugMessage('Common postfix of ' + str(len(inputFiles)) + ' is \'' + common + '\'')
  return common

