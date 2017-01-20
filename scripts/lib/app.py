
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
  import lib.message
  import lib.mrtrix
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

  lib.mrtrix.initialise()

  use_colour = True
  if 'TerminalColor' in lib.mrtrix.config:
    use_colour = lib.mrtrix.config['TerminalColor'].lower() in ('yes', 'true', '1')
  if use_colour:
    lib.message.clearLine = '\033[0K'
    lib.message.colourClear = '\033[0m'
    lib.message.colourConsole = '\033[03;32m'
    lib.message.colourDebug = '\033[03;34m'
    lib.message.colourError = '\033[01;31m'
    lib.message.colourExec = '\033[03;36m'
    lib.message.colourWarn = '\033[00;31m'

  if args.nocleanup:
    cleanup = False
  if args.nthreads:
    lib.mrtrix.optionNThreads = ' -nthreads ' + args.nthreads
  if args.quiet:
    verbosity = 0
    lib.mrtrix.optionVerbosity = ' -quiet'
  elif args.verbose:
    verbosity = 2
    lib.mrtrix.optionVerbosity = ''
  elif args.debug:
    verbosity = 3
    lib.mrtrix.optionVerbosity = ' -info'

  if citationList:
    lib.message.console('')
    citation_warning = 'Note that this script makes use of commands / algorithms that have relevant articles for citation'
    if externalCitations:
      citation_warning += '; INCLUDING FROM EXTERNAL SOFTWARE PACKAGES'
    citation_warning += '. Please consult the help page (-help option) for more information.'
    lib.message.console(citation_warning)
    lib.message.console('')

  if args.cont:
    tempDir = os.path.abspath(args.cont[0])
    lastFile = args.cont[1]



def checkOutputFile(path):
  import os
  import lib.message
  import lib.mrtrix
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
      lib.message.warn('Output' + type + ' ' + os.path.basename(path) + ' already exists; will be overwritten at script completion')
      lib.mrtrix.optionForce = ' -force'
    else:
      lib.message.error('Output' + type + ' ' + path + ' already exists (use -force to override)')



def makeTempDir():
  import os, random, string, sys
  import lib.message
  import lib.mrtrix
  global args, tempDir, workingDir
  if args.cont:
    lib.message.console('Skipping temporary directory creation due to use of -continue option')
    return
  if tempDir:
    lib.message.error('Script error: Cannot use multiple temporary directories')
  if args.tempdir:
    dir_path = os.path.abspath(args.tempdir)
  else:
    if 'TmpFileDir' in lib.mrtrix.config:
      dir_path = lib.mrtrix.config['TmpFileDir']
    else:
      if os.name == 'posix':
        dir_path = '/tmp'
      else:
        dir_path = workingDir
  if 'TmpFilePrefix' in lib.mrtrix.config:
    prefix = lib.mrtrix.config['TmpFilePrefix']
  else:
    prefix = os.path.basename(sys.argv[0]) + '-tmp-'
  tempDir = dir_path
  while os.path.isdir(tempDir):
    random_string = ''.join(random.choice(string.ascii_uppercase + string.digits) for x in range(6))
    tempDir = os.path.join(dir_path, prefix + random_string) + os.sep
  os.makedirs(tempDir)
  lib.message.console('Generated temporary directory: ' + tempDir)
  with open(os.path.join(tempDir, 'cwd.txt'), 'w') as outfile:
    outfile.write(workingDir + '\n')
  with open(os.path.join(tempDir, 'command.txt'), 'w') as outfile:
    outfile.write(' '.join(sys.argv) + '\n')
  open(os.path.join(tempDir, 'log.txt'), 'w').close()



def gotoTempDir():
  import os
  import lib.message
  global tempDir
  if not tempDir:
    lib.message.error('Script error: No temporary directory location set')
  if verbosity:
    lib.message.console('Changing to temporary directory (' + tempDir + ')')
  os.chdir(tempDir)



def complete():
  import os, shutil, sys
  import lib.message
  lib.message.console('Changing back to original directory (' + workingDir + ')')
  os.chdir(workingDir)
  if cleanup and tempDir:
    lib.message.console('Deleting temporary directory ' + tempDir)
    shutil.rmtree(tempDir)
  elif tempDir:
    # This needs to be printed even if the -quiet option is used
    if os.path.isfile(os.path.join(tempDir, 'error.txt')):
      with open(os.path.join(tempDir, 'error.txt'), 'r') as errortext:
        sys.stderr.write(os.path.basename(sys.argv[0]) + ': ' + lib.message.colourWarn + 'Script failed while executing the command: ' + errortext.readline().rstrip() + lib.message.colourClear + '\n')
      sys.stderr.write(os.path.basename(sys.argv[0]) + ': ' + lib.message.colourWarn + 'For debugging, inspect contents of temporary directory: ' + tempDir + lib.message.colourClear + '\n')
    else:
      sys.stderr.write(os.path.basename(sys.argv[0]) + ': ' + lib.message.colourConsole + 'Contents of temporary directory kept, location: ' + tempDir + lib.message.colourClear + '\n')
    sys.stderr.flush()

