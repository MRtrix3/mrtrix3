# This is the primary file that should be included by all Python scripts aiming to use the
#   functionality provided by the MRtrix3 Python script library.
#
# Functions and variables defined here must be called / filled in the appropriate order
#   for the command-line interface to be set up correctly:
# - init()
# - cmdline.addCitation(), cmdline.addDescription(), cmdline.setCopyright() as needed
# - Add arguments and options to 'cmdline' as needed
# - parse()
# - checkOutputPath() as needed
# - makeTempDir() if the script requires a temporary directory
# - gotoTempDir() if the script is using a temporary directory
# - complete()
#



# These global variables can / should be accessed directly by scripts
# - 'cmdline' needs to have the compulsory arguments and optional command-line inputs
#   necessary for the script to be added manually
# - Upon parsing the command-line, the user's command-line inputs will be stored and
#   made accessible through 'args'
# - 'config' contains those entries present in the MRtrix config files
# - 'force' will be True if the user has requested for existing output files to be
#   re-written, and at least one output target already exists
# - 'mrtrix' provides functionality for interfacing with other MRtrix3 components
args = None
cleanup = True
cmdline = None
config = { }
continueOption = False
forceOverwrite = False #pylint: disable=unused-variable
numThreads = None #pylint: disable=unused-variable
tempDir = ''
verbosity = 1 # 0 = quiet; 1 = default; 2 = info; 3 = debug
workingDir = ''






clearLine = ''
colourClear = ''
colourConsole = ''
colourDebug = ''
colourError = ''
colourExec = '' #pylint: disable=unused-variable
colourWarn = ''





_defaultCopyright = '''Copyright (c) 2008-2018 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/

MRtrix3 is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/'''



_signals = { 'SIGALRM': 'Timer expiration',
             'SIGBUS' : 'Bus error: Accessing invalid address (out of storage space?)',
             'SIGFPE' : 'Floating-point arithmetic exception',
             'SIGHUP' : 'Disconnection of terminal',
             'SIGILL' : 'Illegal instruction (corrupt binary command file?)',
             'SIGINT' : 'Program manually interrupted by terminal',
             'SIGPIPE': 'Nothing on receiving end of pipe',
             'SIGPWR' : 'Power failure restart',
             'SIGQUIT': 'Received terminal quit signal',
             'SIGSEGV': 'Segmentation fault: Invalid memory reference',
             'SIGSYS' : 'Bad system call',
             'SIGTERM': 'Terminated by kill command',
             'SIGXCPU': 'CPU time limit exceeded',
             'SIGXFSZ': 'File size limit exceeded' }
           # Can't be handled; see https://bugs.python.org/issue9524
           # 'CTRL_C_EVENT': 'Terminated by user Ctrl-C input',
           # 'CTRL_BREAK_EVENT': Terminated by user Ctrl-Break input'





def init(author, synopsis): #pylint: disable=unused-variable
  import os, signal
  global cmdline, config, workingDir
  cmdline = Parser(author=author, synopsis=synopsis)
  workingDir = os.getcwd()
  # Load the MRtrix configuration files here, and create a dictionary
  # Load system config first, user second: Allows user settings to override
  for path in [ os.environ.get ('MRTRIX_CONFIGFILE', os.path.join(os.path.sep, 'etc', 'mrtrix.conf')),
                os.path.join(os.path.expanduser('~'), '.mrtrix.conf') ]:
    try:
      f = open (path, 'r')
      for line in f:
        line = line.strip().split(': ')
        if len(line) != 2:
          continue
        if line[0][0] == '#':
          continue
        config[line[0]] = line[1]
    except IOError:
      pass
  # Set up signal handlers
  for s in _signals:
    try:
      signal.signal(getattr(signal, s), handler)
    except:
      pass



def parse(): #pylint: disable=unused-variable
  import os, sys
  global args, cleanup, cmdline, continueOption, numThreads, tempDir, verbosity
  global clearLine, colourClear, colourConsole, colourDebug, colourError, colourExec, colourWarn
  from mrtrix3 import run

  if not cmdline:
    sys.stderr.write('Script error: app.init() must be called before app.parse()\n')
    sys.exit(1)

  if len(sys.argv) == 1:
    cmdline.printHelp()
    sys.exit(0)
  elif sys.argv[-1] == '__print_full_usage__':
    cmdline.printFullUsage()
    sys.exit(0)
  elif sys.argv[-1] == '__print_synopsis__':
    sys.stdout.write(cmdline.synopsis)
    sys.exit(0)
  elif sys.argv[-1] == '__print_usage_markdown__':
    cmdline.printUsageMarkdown()
    sys.exit(0)
  elif sys.argv[-1] == '__print_usage_rst__':
    cmdline.printUsageRst()
    sys.exit(0)

  args = cmdline.parse_args()

  if hasattr(args, 'help') and args.help:
    cmdline.printHelp()
    sys.exit(0)

  use_colour = True
  if 'TerminalColor' in config:
    use_colour = config['TerminalColor'].lower() in ('yes', 'true', '1')
  if not sys.stderr.isatty():
    use_colour = False
  if use_colour:
    clearLine = '\033[0K'
    colourClear = '\033[0m'
    colourConsole = '\033[03;32m'
    colourDebug = '\033[03;34m'
    colourError = '\033[01;31m'
    colourExec = '\033[03;36m' #pylint: disable=unused-variable
    colourWarn = '\033[00;31m'

  # Need to check for the presence of these keys first, since there's a chance that an external script may have
  #   erased the standard options
  if hasattr(args, 'nocleanup') and args.nocleanup:
    cleanup = False
  if hasattr(args, 'nthreads') and args.nthreads:
    numThreads = args.nthreads #pylint: disable=unused-variable
  if hasattr(args, 'quiet') and args.quiet:
    verbosity = 0
  elif hasattr(args, 'info') and args.info:
    verbosity = 2
  elif hasattr(args, 'debug') and args.debug:
    verbosity = 3

  cmdline.printCitationWarning()

  if hasattr(args, 'cont') and args.cont:
    continueOption = True
    tempDir = os.path.abspath(args.cont[0])
    # Prevent error from re-appearing at end of terminal output if script continuation results in success
    #   and -nocleanup is used
    try:
      os.remove(os.path.join(tempDir, 'error.txt'))
    except OSError:
      pass
    run.setContinue(args.cont[1])



def checkOutputPath(path): #pylint: disable=unused-variable
  import os
  global args, forceOverwrite, workingDir
  if not path:
    return
  abspath = os.path.abspath(os.path.join(workingDir, path))
  if os.path.exists(abspath):
    output_type = ''
    if os.path.isfile(abspath):
      output_type = ' file'
    elif os.path.isdir(abspath):
      output_type = ' directory'
    if hasattr(args, 'force') and args.force:
      warn('Output' + output_type + ' \'' + path + '\' already exists; will be overwritten at script completion')
      forceOverwrite = True #pylint: disable=unused-variable
    else:
      error('Output' + output_type + ' \'' + path + '\' already exists (use -force to override)')



def makeTempDir(): #pylint: disable=unused-variable
  import os, random, string, sys
  global args, config, continueOption, tempDir, workingDir
  if continueOption:
    debug('Skipping temporary directory creation due to use of -continue option')
    return
  if tempDir:
    error('Script error: Cannot use multiple temporary directories')
  if hasattr(args, 'tempdir') and args.tempdir:
    dir_path = os.path.abspath(args.tempdir)
  else:
    if 'ScriptTmpDir' in config:
      dir_path = config['ScriptTmpDir']
    else:
      # Defaulting to working directory since too many users have encountered storage issues
      dir_path = workingDir
  if 'ScriptTmpPrefix' in config:
    prefix = config['ScriptTmpPrefix']
  else:
    prefix = os.path.basename(sys.argv[0]) + '-tmp-'
  tempDir = dir_path
  while os.path.isdir(tempDir):
    random_string = ''.join(random.choice(string.ascii_uppercase + string.digits) for x in range(6))
    tempDir = os.path.join(dir_path, prefix + random_string) + os.sep
  os.makedirs(tempDir)
  console('Generated temporary directory: ' + tempDir)
  with open(os.path.join(tempDir, 'cwd.txt'), 'w') as outfile:
    outfile.write(workingDir + '\n')
  with open(os.path.join(tempDir, 'command.txt'), 'w') as outfile:
    outfile.write(' '.join(sys.argv) + '\n')
  open(os.path.join(tempDir, 'log.txt'), 'w').close()



def gotoTempDir(): #pylint: disable=unused-variable
  import os
  global tempDir
  if not tempDir:
    error('Script error: No temporary directory location set')
  if verbosity:
    console('Changing to temporary directory (' + tempDir + ')')
  os.chdir(tempDir)



def complete(): #pylint: disable=unused-variable
  import os, shutil, sys
  global cleanup, tempDir, workingDir
  global colourClear, colourConsole, colourWarn
  if os.getcwd() != workingDir:
    console('Changing back to original directory (' + workingDir + ')')
    os.chdir(workingDir)
  if cleanup and tempDir:
    console('Deleting temporary directory ' + tempDir)
    shutil.rmtree(tempDir)
  elif tempDir:
    # This needs to be printed even if the -quiet option is used
    if os.path.isfile(os.path.join(tempDir, 'error.txt')):
      with open(os.path.join(tempDir, 'error.txt'), 'r') as errortext:
        sys.stderr.write(os.path.basename(sys.argv[0]) + ': ' + colourWarn + 'Following command failed during execution of the script:' + colourClear + '\n')
        sys.stderr.write(os.path.basename(sys.argv[0]) + ': ' + colourWarn + errortext.readline().rstrip() + colourClear + '\n')
      sys.stderr.write(os.path.basename(sys.argv[0]) + ': ' + colourWarn + 'For debugging, inspect contents of temporary directory: ' + tempDir + colourClear + '\n')
    else:
      sys.stderr.write(os.path.basename(sys.argv[0]) + ': ' + colourConsole + 'Contents of temporary directory kept, location: ' + tempDir + colourClear + '\n')
    sys.stderr.flush()






# This function should be used to insert text into any mrconvert call writing an output image
#   to the user's requested destination
# It will ensure that the header contents of any output images reflect the execution of the script itself,
#   rather than its internal processes
def mrconvertOutputOption(input_image): #pylint: disable=unused-variable
  import sys
  from ._version import __version__
  global forceOverwrite
  s = ' -compel_keyvalues ' + input_image + ' "' + sys.argv[0]
  for arg in sys.argv[1:]:
    s += ' \\"' + arg + '\\"'
  s += '  (version=' + __version__ + ')"'
  if forceOverwrite:
    s += ' -force'
  return s






# A set of functions and variables for printing various information at the command-line.
def console(text): #pylint: disable=unused-variable
  import os, sys
  global colourClear, colourConsole
  global verbosity
  if verbosity:
    sys.stderr.write(os.path.basename(sys.argv[0]) + ': ' + colourConsole + text + colourClear + '\n')

def debug(text): #pylint: disable=unused-variable
  import inspect, os, sys
  global colourClear, colourDebug
  global verbosity
  if verbosity <= 2:
    return
  outer_frames = inspect.getouterframes(inspect.currentframe())
  nearest = outer_frames[1]
  try:
    if len(outer_frames) == 2: # debug() called directly from script being executed
      try:
        origin = '(' + os.path.basename(nearest.filename) + ':' + str(nearest.lineno) + ')'
      except AttributeError: # Prior to Python 3.5
        origin = '(' + os.path.basename(nearest[1]) + ':' + str(nearest[2]) + ')'
    else: # Some function has called debug(): Get location of both that function, and where that function was invoked
      try:
        filename = nearest.filename
        funcname = nearest.function + '()'
      except: # Prior to Python 3.5
        filename = nearest[1]
        funcname = nearest[3] + '()'
      modulename = inspect.getmodulename(filename)
      if modulename:
        funcname = modulename + '.' + funcname
      origin = funcname
      caller = outer_frames[2]
      try:
        origin += ' (from ' + os.path.basename(caller.filename) + ':' + str(caller.lineno) + ')'
      except AttributeError:
        origin += ' (from ' + os.path.basename(caller[1]) + ':' + str(caller[2]) + ')'
      finally:
        del caller
    sys.stderr.write(os.path.basename(sys.argv[0]) + ': ' + colourDebug + '[DEBUG] ' + origin + ': ' + text + colourClear + '\n')
  finally:
    del nearest

def error(text): #pylint: disable=unused-variable
  import os, sys
  global colourClear, colourError
  global cleanup
  sys.stderr.write(os.path.basename(sys.argv[0]) + ': ' + colourError + '[ERROR] ' + text + colourClear + '\n')
  cleanup = False
  complete()
  sys.exit(1)

def var(*variables): #pylint: disable=unused-variable
  import inspect, os, sys
  global colourClear, colourDebug
  global verbosity
  if verbosity <= 2:
    return
  calling_frame = inspect.getouterframes(inspect.currentframe())[1]
  try:
    try:
      calling_code = calling_frame.code_context[0]
      filename = calling_frame.filename
      lineno = calling_frame.lineno
    except AttributeError: # Prior to Python 3.5
      calling_code = calling_frame[4][0]
      filename = calling_frame[1]
      lineno = calling_frame[2]
    var_string = calling_code[calling_code.find('var(')+4:].rstrip('\n').rstrip(' ')[:-1].replace(',', ' ')
    var_names, var_values = var_string.split(), variables
    for name, value in zip(var_names, var_values):
      sys.stderr.write(os.path.basename(sys.argv[0]) + ': ' + colourDebug + '[DEBUG] (from ' + os.path.basename(filename) + ':' + str(lineno) + ') \'' + name + '\' = ' + str(value) + colourClear + '\n')
  finally:
    del calling_frame

def warn(text): #pylint: disable=unused-variable
  import os, sys
  global colourClear, colourWarn
  sys.stderr.write(os.path.basename(sys.argv[0]) + ': ' + colourWarn + '[WARNING] ' + text + colourClear + '\n')






# The Parser class is responsible for setting up command-line parsing for the script.
#   This includes proper configuration of the argparse functionality, adding standard options
#   that are common for all scripts, providing a custom help page that is consistent with the
#   MRtrix3 binaries, and defining functions for exporting the help page for the purpose of
#   automated self-documentation.

import argparse
class Parser(argparse.ArgumentParser):

  #pylint: disable=protected-access

  def __init__(self, *args_in, **kwargs_in):
    global _defaultCopyright
    if 'author' in kwargs_in:
      self._author = kwargs_in['author']
      del kwargs_in['author']
    else:
      self._author = None
    self.citationList = [ ]
    if 'copyright' in kwargs_in:
      self._copyright = kwargs_in['copyright']
      del kwargs_in['copyright']
    else:
      self._copyright = _defaultCopyright
    self._description = [ ]
    self.externalCitations = False
    if 'synopsis' in kwargs_in:
      self.synopsis = kwargs_in['synopsis']
      del kwargs_in['synopsis']
    else:
      self.synopsis = None
    kwargs_in['add_help'] = False
    argparse.ArgumentParser.__init__(self, *args_in, **kwargs_in)
    self.mutuallyExclusiveOptionGroups = [ ]
    if 'parents' in kwargs_in:
      for parent in kwargs_in['parents']:
        self.citationList.extend(parent.citationList)
        self.externalCitations = self.externalCitations or parent.externalCitations
    else:
      standard_options = self.add_argument_group('Standard options')
      standard_options.add_argument('-continue', nargs=2, dest='cont', metavar=('<TempDir>', '<LastFile>'), help='Continue the script from a previous execution; must provide the temporary directory path, and the name of the last successfully-generated file')
      standard_options.add_argument('-force', action='store_true', help='Force overwrite of output files if pre-existing')
      standard_options.add_argument('-help', action='store_true', help='Display help information for the script')
      standard_options.add_argument('-nocleanup', action='store_true', help='Do not delete temporary files during script, or temporary directory at script completion')
      standard_options.add_argument('-nthreads', metavar='number', help='Use this number of threads in MRtrix multi-threaded applications (0 disables multi-threading)')
      standard_options.add_argument('-tempdir', metavar='/path/to/tmp/', help='Manually specify the path in which to generate the temporary directory')
      standard_options.add_argument('-quiet',   action='store_true', help='Suppress all console output during script execution')
      standard_options.add_argument('-info', action='store_true', help='Display additional information and progress for every command invoked')
      standard_options.add_argument('-debug', action='store_true', help='Display additional debugging information over and above the output of -info')
      self.flagMutuallyExclusiveOptions( [ 'quiet', 'info', 'debug' ] )

  def addCitation(self, condition, reference, is_external): #pylint: disable=unused-variable
    self.citationList.append( (condition, reference) )
    if is_external:
      self.externalCitations = True

  def addDescription(self, text): #pylint: disable=unused-variable
    self._description.append(text)

  def setCopyright(self, text): #pylint: disable=unused-variable
    self._copyright = text

  # Mutually exclusive options need to be added before the command-line input is parsed
  def flagMutuallyExclusiveOptions(self, options, required=False): #pylint: disable=unused-variable
    import sys
    if not isinstance(options, list) or not isinstance(options[0], str):
      sys.stderr.write('Script error: Parser.flagMutuallyExclusiveOptions() only accepts a list of strings\n')
      sys.stderr.flush()
      sys.exit(1)
    self.mutuallyExclusiveOptionGroups.append( (options, required) )

  def parse_args(self):
    result = argparse.ArgumentParser.parse_args(self)
    self.checkMutuallyExclusiveOptions(result)
    if self._subparsers:
      for alg in self._subparsers._group_actions[0].choices:
        self._subparsers._group_actions[0].choices[alg].checkMutuallyExclusiveOptions(result)
    return result

  def printCitationWarning(self):
    # If a subparser has been invoked, the subparser's function should instead be called,
    #   since it might have had additional citations appended
    global args
    if self._subparsers:
      subparser = getattr(args, self._subparsers._group_actions[0].dest)
      for alg in self._subparsers._group_actions[0].choices:
        if alg == subparser:
          self._subparsers._group_actions[0].choices[alg].printCitationWarning()
          return
    if self.citationList:
      console('')
      citation_warning = 'Note that this script makes use of commands / algorithms that have relevant articles for citation'
      if self.externalCitations:
        citation_warning += '; INCLUDING FROM EXTERNAL SOFTWARE PACKAGES'
      citation_warning += '. Please consult the help page (-help option) for more information.'
      console(citation_warning)
      console('')

  # Overloads argparse.ArgumentParser function to give a better error message on failed parsing
  def error(self, text):
    import shlex, sys
    for entry in sys.argv:
      if '-help'.startswith(entry):
        self.printHelp()
        sys.exit(0)
    if self.prog and len(shlex.split(self.prog)) == len(sys.argv): # No arguments provided to subparser
      self.printHelp()
      sys.exit(0)
    usage = self.formatUsage()
    if self._subparsers:
      for alg in self._subparsers._group_actions[0].choices:
        if alg == sys.argv[1]:
          usage = self._subparsers._group_actions[0].choices[alg].formatUsage()
          continue
    sys.stderr.write('\nError: %s\n' % text)
    sys.stderr.write('Usage: ' + usage + '\n')
    sys.stderr.write('       (Run ' + self.prog + ' -help for more information)\n\n')
    sys.stderr.flush()
    sys.exit(1)

  def checkMutuallyExclusiveOptions(self, args_in):
    import sys
    for group in self.mutuallyExclusiveOptionGroups:
      count = 0
      for option in group[0]:
        # Checking its presence is not adequate; by default, argparse adds these members to the namespace
        # Need to test if more than one of these options DIFFERS FROM ITS DEFAULT
        # Will need to loop through actions to find it manually
        if hasattr(args_in, option):
          for arg in self._actions:
            if arg.dest == option:
              if not getattr(args_in, option) == arg.default:
                count += 1
              break
      if count > 1:
        sys.stderr.write('\nError: You cannot use more than one of the following options: ' + ', '.join([ '-' + o for o in group[0] ]) + '\n')
        sys.stderr.write('(Consult the help page for more information: ' + self.prog + ' -help)\n\n')
        sys.stderr.flush()
        sys.exit(1)
      if group[1] and not count:
        sys.stderr.write('\nError: One of the following options must be provided: ' + ', '.join([ '-' + o for o in group[0] ]) + '\n')
        sys.stderr.write('(Consult the help page for more information: ' + self.prog + ' -help)\n\n')
        sys.stderr.flush()
        sys.exit(1)

  def formatUsage(self):
    argument_list = [ ]
    trailing_ellipsis = ''
    if self._subparsers:
      argument_list.append(self._subparsers._group_actions[0].dest)
      trailing_ellipsis = ' ...'
    for arg in self._positionals._group_actions:
      if arg.metavar:
        argument_list.append(arg.metavar)
      else:
        argument_list.append(arg.dest)
    return self.prog + ' ' + ' '.join(argument_list) + ' [ options ]' + trailing_ellipsis

  def printHelp(self):
    import subprocess, sys, textwrap

    def bold(text):
      return ''.join( c + chr(0x08) + c for c in text)

    def underline(text):
      return ''.join( '_' + chr(0x08) + c for c in text)

    def appVersion():
      import subprocess, os
      from mrtrix3.run import _mrtrix_bin_path
      p = subprocess.Popen([os.path.join(_mrtrix_bin_path,'mrinfo'),'-version'],stdout=subprocess.PIPE)
      line = p.stdout.readline().decode()
      return line.replace('==','').replace('mrinfo','').lstrip().rstrip()

    w = textwrap.TextWrapper(width=80, initial_indent='     ', subsequent_indent='     ')
    w_arg = textwrap.TextWrapper(width=80, initial_indent='', subsequent_indent='                     ')
    from ._version import __version__

    s = 'MRtrix ' + __version__ + '\t' + bold(self.prog) + '\t bin version: ' + appVersion() + '\n\n'
    s += '\n'
    s += '     ' + bold(self.prog) + ': Script using the MRtrix3 Python library\n'
    s += '\n'
    s += bold('SYNOPSIS') + '\n'
    s += '\n'
    s += w.fill(self.synopsis) + '\n'
    s += '\n'
    s += bold('USAGE') + '\n'
    s += '\n'
    usage = self.prog + ' [ options ]'
    # Compulsory sub-cmdline algorithm selection (if present)
    if self._subparsers:
      usage += ' ' + self._subparsers._group_actions[0].dest + ' ...'
    # Find compulsory input arguments
    for arg in self._positionals._group_actions:
      if arg.metavar:
        usage += ' ' + arg.metavar
      else:
        usage += ' ' + arg.dest
    # Unfortunately this can line wrap early because textwrap is counting each
    #   underlined character as 3 characters when calculating when to wrap
    # Fix by underlining after the fact
    s += w.fill(usage).replace(self.prog, underline(self.prog), 1) + '\n'
    s += '\n'
    if self._subparsers:
      s += '        ' + w_arg.fill(self._subparsers._group_actions[0].dest + ' '*(max(13-len(self._subparsers._group_actions[0].dest), 1)) + self._subparsers._group_actions[0].help).replace (self._subparsers._group_actions[0].dest, underline(self._subparsers._group_actions[0].dest), 1) + '\n'
      s += '\n'
    for arg in self._positionals._group_actions:
      line = '        '
      if arg.metavar:
        name = arg.metavar
      else:
        name = arg.dest
      line += name + ' '*(max(13-len(name), 1)) + arg.help
      s += w_arg.fill(line).replace(name, underline(name), 1) + '\n'
      s += '\n'
    if self._description:
      s += bold('DESCRIPTION') + '\n'
      s += '\n'
      for line in self._description:
        s += w.fill(line) + '\n'
        s += '\n'
    # Option groups
    for group in reversed(self._action_groups):
      # * Don't display empty groups
      # * Don't display the subcmdline option; that's dealt with in the usage
      # * Don't re-display any compulsory positional arguments; they're also dealt with in the usage
      if group._group_actions and not (len(group._group_actions) == 1 and isinstance(group._group_actions[0], argparse._SubParsersAction)) and not group == self._positionals:
        s += bold(group.title) + '\n'
        s += '\n'
        for option in group._group_actions:
          s += '  ' + underline('/'.join(option.option_strings))
          if option.metavar:
            s += ' '
            if isinstance(option.metavar, tuple):
              s += ' '.join(option.metavar)
            else:
              s += option.metavar
          elif option.nargs:
            if isinstance(option.nargs, int):
              s += (' ' + option.dest.upper())*option.nargs
            elif option.nargs == '+' or option.nargs == '*':
              s += ' <space-separated list>'
            elif option.nargs == '?':
              s += ' <optional value>'
          elif option.type is not None:
            s += ' ' + option.type.__name__.upper()
          elif option.default is None:
            s += ' ' + option.dest.upper()
          # Any options that haven't tripped one of the conditions above should be a store_true or store_false, and
          #   therefore there's nothing to be appended to the option instruction
          s += '\n'
          s += w.fill(option.help) + '\n'
          s += '\n'
    s += bold('AUTHOR') + '\n'
    s += w.fill(self._author) + '\n'
    s += '\n'
    s += bold('COPYRIGHT') + '\n'
    s += w.fill(self._copyright) + '\n'
    if self.citationList:
      s += '\n'
      s += bold('REFERENCES') + '\n'
      s += '\n'
      for entry in self.citationList:
        if entry[0]:
          s += w.fill('* ' + entry[0] + ':') + '\n'
        s += w.fill(entry[1]) + '\n'
        s += '\n'
    if 'HelpCommand' in config:
      command = config['HelpCommand']
    else:
      command = 'less -X'
    if command:
      try:
        process = subprocess.Popen(command.split(' '), stdin=subprocess.PIPE)
        process.communicate(s.encode())
      except:
        sys.stdout.write(s)
        sys.stdout.flush()
    else:
      sys.stdout.write(s)
      sys.stdout.flush()

  def printFullUsage(self):
    import sys
    sys.stdout.write(self.synopsis + '\n')
    if self._description:
      if isinstance(self._description, list):
        for line in self._description:
          sys.stdout.write(line + '\n')
      else:
        sys.stdout.write(self._description + '\n')
    if self._subparsers and len(sys.argv) == 3:
      for alg in self._subparsers._group_actions[0].choices:
        if alg == sys.argv[1]:
          self._subparsers._group_actions[0].choices[alg].printFullUsage()
          return
      self.error('Invalid subparser nominated')
    for arg in self._positionals._group_actions:
      # This will need updating if any scripts allow mulitple argument inputs
      sys.stdout.write('ARGUMENT ' + arg.dest + ' 0 0\n')
      sys.stdout.write(arg.help + '\n')
    for group in reversed(self._action_groups):
      if group._group_actions and not (len(group._group_actions) == 1 and isinstance(group._group_actions[0], argparse._SubParsersAction)) and not group == self._positionals:
        for option in group._group_actions:
          sys.stdout.write('OPTION ' + '/'.join(option.option_strings) + ' 1 0\n')
          sys.stdout.write(option.help + '\n')
          if option.metavar:
            if isinstance(option.metavar, tuple):
              for arg in option.metavar:
                sys.stdout.write('ARGUMENT ' + arg + ' 0 0\n')
            else:
              sys.stdout.write('ARGUMENT ' + option.metavar + ' 0 0\n')
    sys.stdout.flush()

  def printUsageMarkdown(self):
    import os, subprocess, sys
    if self._subparsers and len(sys.argv) == 3:
      for alg in self._subparsers._group_actions[0].choices:
        if alg == sys.argv[-2]:
          self._subparsers._group_actions[0].choices[alg].printUsageMarkdown()
          return
      self.error('Invalid subcmdline nominated')
    s = '## Synopsis\n\n'
    s += self.synopsis + '\n\n'
    s += '## Usage\n\n'
    s += '    ' + self.formatUsage() + '\n\n'
    if self._subparsers:
      s += '-  *' + self._subparsers._group_actions[0].dest + '*: ' + self._subparsers._group_actions[0].help + '\n'
    for arg in self._positionals._group_actions:
      if arg.metavar:
        name = arg.metavar
      else:
        name = arg.dest
      s += '-  *' + name + '*: ' + arg.help + '\n\n'
    if self._description:
      s += '## Description\n\n'
      for line in self._description:
        s += line + '\n\n'
    s += '## Options\n\n'
    for group in reversed(self._action_groups):
      if group._group_actions and not (len(group._group_actions) == 1 and isinstance(group._group_actions[0], argparse._SubParsersAction)) and not group == self._positionals:
        s += '#### ' + group.title + '\n\n'
        for option in group._group_actions:
          text = '/'.join(option.option_strings)
          if option.metavar:
            text += ' '
            if isinstance(option.metavar, tuple):
              text += ' '.join(option.metavar)
            else:
              text += option.metavar
          s += '+ **-' + text + '**<br>' + option.help + '\n\n'
    if self.citationList:
      s += '## References\n\n'
      for ref in self.citationList:
        text = ''
        if ref[0]:
          text += ref[0] + ': '
        text += ref[1]
        s += text + '\n\n'
    s += '---\n\n'
    s += '**Author:** ' + self._author + '\n\n'
    s += '**Copyright:** ' + self._copyright + '\n\n'
    sys.stdout.write(s)
    sys.stdout.flush()
    if self._subparsers:
      for alg in self._subparsers._group_actions[0].choices:
        subprocess.call ([ sys.executable, os.path.realpath(sys.argv[0]), alg, '__print_usage_markdown__' ])

  def printUsageRst(self):
    import os, subprocess, sys
    # Need to check here whether it's the documentation for a particular subcmdline that's being requested
    if self._subparsers and len(sys.argv) == 3:
      for alg in self._subparsers._group_actions[0].choices:
        if alg == sys.argv[-2]:
          self._subparsers._group_actions[0].choices[alg].printUsageRst()
          return
      self.error('Invalid subparser nominated: ' + sys.argv[-2])
    s = '.. _' + self.prog.replace(' ', '_') + ':\n\n'
    s += self.prog + '\n'
    s += '='*len(self.prog) + '\n\n'
    s += 'Synopsis\n'
    s += '--------\n\n'
    s += self.synopsis + '\n\n'
    s += 'Usage\n'
    s += '--------\n\n'
    s += '::\n\n'
    s += '    ' + self.formatUsage() + '\n\n'
    if self._subparsers:
      s += '-  *' + self._subparsers._group_actions[0].dest + '*: ' + self._subparsers._group_actions[0].help + '\n'
    for arg in self._positionals._group_actions:
      if arg.metavar:
        name = arg.metavar
      else:
        name = arg.dest
      s += '-  *' + name + '*: ' + arg.help.replace('|', '\\|') + '\n'
    s += '\n'
    if self._description:
      s += 'Description\n'
      s += '-----------\n\n'
      for line in self._description:
        s += line + '\n\n'
    s += 'Options\n'
    s += '-------\n'
    for group in reversed(self._action_groups):
      if group._group_actions and not (len(group._group_actions) == 1 and isinstance(group._group_actions[0], argparse._SubParsersAction)) and not group == self._positionals:
        s += '\n'
        s += group.title + '\n'
        s += '^'*len(group.title) + '\n'
        for option in group._group_actions:
          text = '/'.join(option.option_strings)
          if option.metavar:
            text += ' '
            if isinstance(option.metavar, tuple):
              text += ' '.join(option.metavar)
            else:
              text += option.metavar
          s += '\n'
          s += '- **' + text + '** ' + option.help.replace('|', '\\|') + '\n'
    if self.citationList:
      s += '\n'
      s += 'References\n'
      s += '^^^^^^^^^^\n'
      for ref in self.citationList:
        text = '* '
        if ref[0]:
          text += ref[0] + ': '
        text += ref[1]
        s += '\n'
        s += text + '\n'
    s += '\n'
    s += '--------------\n\n\n\n'
    s += '**Author:** ' + self._author + '\n\n'
    s += '**Copyright:** ' + self._copyright + '\n\n'
    sys.stdout.write(s)
    sys.stdout.flush()
    if self._subparsers:
      for alg in self._subparsers._group_actions[0].choices:
        subprocess.call ([ sys.executable, os.path.realpath(sys.argv[0]), alg, '__print_usage_rst__' ])



# A class that can be used to display a progress bar on the terminal,
#   mimicing the behaviour of MRtrix3 binary commands
class progressBar(object): #pylint: disable=unused-variable

  _busy = [ '.   ',
            ' .  ',
            '  . ',
            '   .',
            '  . ',
            ' .  ' ]

  def _update(self):
    import sys
    global clearLine, colourConsole, colourClear, colourExec
    assert not self.iscomplete
    if self.isatty:
      sys.stderr.write('\r' + self.scriptname + ': ' + colourExec + '[' + ('{0:>3}%'.format(self.value) if self.multiplier else progressBar._busy[self.counter%6]) + ']' + colourClear + ' ' + colourConsole + self.message + '... ' + colourClear + clearLine + self.newline)
    else:
      if self.newline:
        sys.stderr.write(self.scriptname + ': ' + self.message + '... [' + ('=' * int(self.value/2)) + self.newline)
      else:
        sys.stderr.write('=' * (int(self.value/2) - int(self.old_value/2)))
    sys.stderr.flush()

  def __init__(self, msg, target=0):
    import os, sys
    global verbosity
    self.counter = 0
    self.isatty = sys.stderr.isatty()
    self.iscomplete = False
    self.message = msg
    self.multiplier = 100.0/target if target else 0
    self.newline = '\n' if verbosity > 1 else '' # If any more than default verbosity, may still get details printed in between progress updates
    self.old_value = 0
    self.origverbosity = verbosity
    self.scriptname = os.path.basename(sys.argv[0])
    self.value = 0
    verbosity = verbosity - 1 if verbosity else 0
    if self.isatty:
      sys.stderr.write(self.scriptname + ': ' + colourExec + '[' + ('{0:>3}%'.format(self.value) if self.multiplier else progressBar._busy[0]) + ']' + colourClear + ' ' + colourConsole + self.message + '... ' + colourClear + clearLine + self.newline)
    else:
      sys.stderr.write(self.scriptname + ': ' + self.message + '... [' + self.newline)
    sys.stderr.flush()

  def increment(self, msg=''):
    import math
    assert not self.iscomplete
    self.counter += 1
    force_update = False
    if msg:
      self.message = msg
      force_update = True
    if self.multiplier:
      new_value = int(round(self.counter * self.multiplier))
    else:
      new_value = math.log(self.counter, 2)
    if new_value != self.value:
      self.old_value = self.value
      self.value = new_value
      force_update = True
    if force_update:
      self._update()

  def done(self):
    import sys
    global verbosity
    global clearLine, colourConsole, colourClear, colourExec
    self.iscomplete = True
    self.value = 100
    if self.isatty:
      sys.stderr.write('\r' + self.scriptname + ': ' + colourExec + '[' + ('100%' if self.multiplier else 'done') + ']' + colourClear + ' ' + colourConsole + self.message + colourClear + clearLine + '\n')
    else:
      if self.newline:
        sys.stderr.write(self.scriptname + ': ' + self.message + ' [' + ('=' * (self.value/2)) + ']\n')
      else:
        sys.stderr.write('=' * (int(self.value/2) - int(self.old_value/2)) + ']\n')
    sys.stderr.flush()
    verbosity = self.origverbosity




# Return a boolean flag to indicate whether or not script is being run on a Windows machine
def isWindows(): #pylint: disable=unused-variable
  import platform
  system = platform.system().lower()
  return system.startswith('mingw') or system.startswith('msys') or system.startswith('nt') or system.startswith('windows')




# Handler function for dealing with system signals
def handler(signum, _frame):
  import os, signal, sys
  global _signals
  # Ignore any other incoming signals
  for s in _signals:
    try:
      signal.signal(getattr(signal, s), signal.SIG_IGN)
    except:
      pass
  # Kill any child processes in the run module
  try:
    from mrtrix3.run import _processes
    for p in _processes:
      if p:
        p.terminate()
        p.communicate() # Flushes the I/O buffers
    _processes = [ ]
  except ImportError:
    pass
  # Generate the error message
  msg = '[SYSTEM FATAL CODE: '
  signal_found = False
  for (key, value) in _signals.items():
    try:
      if getattr(signal, key) == signum:
        msg += key + ' (' + str(int(signum)) + ')] ' + value
        signal_found = True
        break
    except AttributeError:
      pass
  if not signal_found:
    msg += '?] Unknown system signal'
  sys.stderr.write('\n' + os.path.basename(sys.argv[0]) + ': ' + colourError + msg + colourClear + '\n')
  complete()
  exit(signum)
