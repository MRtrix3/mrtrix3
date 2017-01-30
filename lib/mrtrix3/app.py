
args = ''
author = ''
citationList = []
cleanup = True
copyright = '''Copyright (c) 2008-2017 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/

MRtrix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see www.mrtrix.org'''
externalCitations = False
lastFile = ''
parser = None
tempDir = ''
verbosity = 1 # 0 = quiet; 1 = default; 2 = verbose; 3 = debug
workingDir = ''




def addCitation(condition, reference, is_external):
  global citationList, externalCitations
  citationList.append( (condition, reference) )
  if is_external:
    externalCitations = True



def initialise():
  import os, sys
  from mrtrix3 import message, mrtrix
  global args, citationList, cleanup, externalCitations, lastFile, parser, tempDir, verbosity, workingDir

  if not parser:
    sys.stderr.write('Script error: Command-line parser must be initialised before app\n')
    sys.exit(1)

  if len(sys.argv) == 1:
    parser.print_help()
    sys.exit(0)

  if sys.argv[-1] == '__print_usage_markdown__':
    parser.printUsageMarkdown()
    sys.exit(0)

  if sys.argv[-1] == '__print_usage_rst__':
    parser.printUsageRst()
    sys.exit(0)

  workingDir = os.getcwd()

  args = parser.parse_args()
  if args.help:
    parser.print_help()
    sys.exit(0)

  mrtrix.initialise()

  use_colour = True
  if 'TerminalColor' in mrtrix.config:
    use_colour = mrtrix.config['TerminalColor'].lower() in ('yes', 'true', '1')
  if use_colour:
    message.clearLine = '\033[0K'
    message.colourClear = '\033[0m'
    message.colourConsole = '\033[03;32m'
    message.colourDebug = '\033[03;34m'
    message.colourError = '\033[01;31m'
    message.colourExec = '\033[03;36m'
    message.colourWarn = '\033[00;31m'

  if args.nocleanup:
    cleanup = False
  if args.nthreads:
    mrtrix.optionNThreads = ' -nthreads ' + args.nthreads
  if args.quiet:
    verbosity = 0
    mrtrix.optionVerbosity = ' -quiet'
  elif args.verbose:
    verbosity = 2
    mrtrix.optionVerbosity = ''
  elif args.debug:
    verbosity = 3
    mrtrix.optionVerbosity = ' -info'

  if citationList:
    message.console('')
    citation_warning = 'Note that this script makes use of commands / algorithms that have relevant articles for citation'
    if externalCitations:
      citation_warning += '; INCLUDING FROM EXTERNAL SOFTWARE PACKAGES'
    citation_warning += '. Please consult the help page (-help option) for more information.'
    message.console(citation_warning)
    message.console('')

  if args.cont:
    tempDir = os.path.abspath(args.cont[0])
    lastFile = args.cont[1]



def checkOutputFile(path):
  import os
  from mrtrix3 import message, mrtrix
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
      message.warn('Output' + type + ' ' + os.path.basename(path) + ' already exists; will be overwritten at script completion')
      mrtrix.optionForce = ' -force'
    else:
      message.error('Output' + type + ' ' + path + ' already exists (use -force to override)')



def makeTempDir():
  import os, random, string, sys
  from mrtrix3 import message, mrtrix
  global args, tempDir, workingDir
  if args.cont:
    message.console('Skipping temporary directory creation due to use of -continue option')
    return
  if tempDir:
    message.error('Script error: Cannot use multiple temporary directories')
  if args.tempdir:
    dir_path = os.path.abspath(args.tempdir)
  else:
    if 'TmpFileDir' in mrtrix.config:
      dir_path = mrtrix.config['TmpFileDir']
    else:
      if os.name == 'posix':
        dir_path = '/tmp'
      else:
        dir_path = workingDir
  if 'TmpFilePrefix' in mrtrix.config:
    prefix = mrtrix.config['TmpFilePrefix']
  else:
    prefix = os.path.basename(sys.argv[0]) + '-tmp-'
  tempDir = dir_path
  while os.path.isdir(tempDir):
    random_string = ''.join(random.choice(string.ascii_uppercase + string.digits) for x in range(6))
    tempDir = os.path.join(dir_path, prefix + random_string) + os.sep
  os.makedirs(tempDir)
  message.console('Generated temporary directory: ' + tempDir)
  with open(os.path.join(tempDir, 'cwd.txt'), 'w') as outfile:
    outfile.write(workingDir + '\n')
  with open(os.path.join(tempDir, 'command.txt'), 'w') as outfile:
    outfile.write(' '.join(sys.argv) + '\n')
  open(os.path.join(tempDir, 'log.txt'), 'w').close()



def gotoTempDir():
  import os
  from mrtrix3 import message
  global tempDir
  if not tempDir:
    message.error('Script error: No temporary directory location set')
  if verbosity:
    message.console('Changing to temporary directory (' + tempDir + ')')
  os.chdir(tempDir)



def complete():
  import os, shutil, sys
  from mrtrix3 import message
  message.console('Changing back to original directory (' + workingDir + ')')
  os.chdir(workingDir)
  if cleanup and tempDir:
    message.console('Deleting temporary directory ' + tempDir)
    shutil.rmtree(tempDir)
  elif tempDir:
    # This needs to be printed even if the -quiet option is used
    if os.path.isfile(os.path.join(tempDir, 'error.txt')):
      with open(os.path.join(tempDir, 'error.txt'), 'r') as errortext:
        sys.stderr.write(os.path.basename(sys.argv[0]) + ': ' + message.colourWarn + 'Script failed while executing the command: ' + errortext.readline().rstrip() + message.colourClear + '\n')
      sys.stderr.write(os.path.basename(sys.argv[0]) + ': ' + message.colourWarn + 'For debugging, inspect contents of temporary directory: ' + tempDir + message.colourClear + '\n')
    else:
      sys.stderr.write(os.path.basename(sys.argv[0]) + ': ' + message.colourConsole + 'Contents of temporary directory kept, location: ' + tempDir + message.colourClear + '\n')
    sys.stderr.flush()

