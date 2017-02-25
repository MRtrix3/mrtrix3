# This is the primary file that should be included by all Python scripts aiming to use the
#   functionality provided by the MRtrix3 Python script library.
#
# Functions and variables defined here must be called / filled in the appropriate order
#   for the command-line interface to be set up correctly:
# - init()
# - cmdline.addCitation(), cmdline.addDescription(), cmdline.setCopyright() as needed
# - Add arguments and options to 'cmdline' as needed
# - parse()
# - checkOutputFile() as needed
# - makeTempDir() if the script requires a temporary directory
# - gotoTempDir() if the script is using a temporary directory
# - complete()
#
# checkOutputFile() can be called at any time after parse() to check if an intended output
#   file already exists



# These global variables can / should be accessed directly by scripts
# - 'cmdline' needs to have the compulsory arguments and optional command-line inputs
#   necessary for the script to be added manually
# - Upon parsing the command-line, the user's command-line inputs will be stored and
#   made accessible through 'args'
# - 'config' contains those entries present in the MRtrix config files
# - 'force' will be True if the user has requested for existing output files to be
#   re-written, and at least one output target already exists
# - 'mrtrix' provides functionality for interfacing with other MRtrix3 components
args = ''
cmdline = None
config = { }
force = False



# These are used to configure the script interface and operation, and should not typically be accessed/modified directly
_cleanup = True
_defaultCopyright = '''Copyright (c) 2008-2017 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/.

MRtrix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/.'''
_lastFile = ''
_nthreads = None
_tempDir = ''
_verbosity = 1 # 0 = quiet; 1 = default; 2 = info; 3 = debug
_workingDir = ''



# Data used for setting up signal handlers
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




def init(author, synopsis):
  import os
  global cmdline, config
  global _signalData, _workingDir
  cmdline = Parser(author=author, synopsis=synopsis)
  _workingDir = os.getcwd()
  # Load the MRtrix configuration files here, and create a dictionary
  # Load system config first, user second: Allows user settings to override
  for path in [ os.path.join(os.path.sep, 'etc', 'mrtrix.conf'),
                os.path.join(os.path.expanduser('~'), '.mrtrix.conf') ]:
    try:
      f = open (path, 'r')
      for line in f:
        line = line.strip().split(': ')
        if len(line) != 2: continue
        if line[0][0] == '#': continue
        config[line[0]] = line[1]
    except IOError:
      pass
  # Set up signal handlers
  for s in _signals:
    try:
      signal.signal(getattr(signal, s), _handler)
    except:
      pass



def parse():
  import os, sys
  global args, cmdline
  global _cleanup, _lastFile, _nthreads, _tempDir, _verbosity
  global clearLine, colourClear, colourConsole, colourDebug, colourError, colourExec, colourWarn

  if not cmdline:
    sys.stderr.write('Script error: app.init() must be called before app.parse()\n')
    sys.exit(1)

  if len(sys.argv) == 1:
    cmdline._printHelp()
    sys.exit(0)

  if sys.argv[-1] == '__print_full_usage__':
    cmdline._printFullUsage()
    sys.exit(0)

  if sys.argv[-1] == '__print_synopsis__':
    sys.stdout.write(cmdline.synopsis)
    sys.exit(0)

  if sys.argv[-1] == '__print_usage_markdown__':
    cmdline._printUsageMarkdown()
    sys.exit(0)

  if sys.argv[-1] == '__print_usage_rst__':
    cmdline._printUsageRst()
    sys.exit(0)

  args = cmdline.parse_args()

  if args.help:
    cmdline._printHelp()
    sys.exit(0)

  use_colour = True
  if 'TerminalColor' in config:
    use_colour = config['TerminalColor'].lower() in ('yes', 'true', '1')
  if use_colour:
    clearLine = '\033[0K'
    colourClear = '\033[0m'
    colourConsole = '\033[03;32m'
    colourDebug = '\033[03;34m'
    colourError = '\033[01;31m'
    colourExec = '\033[03;36m'
    colourWarn = '\033[00;31m'

  if args.nocleanup:
    _cleanup = False
  if args.nthreads:
    _nthreads = args.nthreads
  if args.quiet:
    _verbosity = 0
  elif args.info:
    _verbosity = 2
  elif args.debug:
    _verbosity = 3

  cmdline.printCitationWarning()

  if args.cont:
    _tempDir = os.path.abspath(args.cont[0])
    _lastFile = args.cont[1]




def checkOutputPath(path):
  import os
  global args, force
  global _workingDir
  if not path:
    return
  abspath = os.path.abspath(os.path.join(_workingDir, path))
  if os.path.exists(abspath):
    type = ''
    if os.path.isfile(abspath):
      type = ' file'
    elif os.path.isdir(abspath):
      type = ' directory'
    if args.force:
      warn('Output' + type + ' \'' + path + '\' already exists; will be overwritten at script completion')
      force = True
    else:
      error('Output' + type + ' \'' + path + '\' already exists (use -force to override)')




def makeTempDir():
  import os, random, string, sys
  global args, config
  global _tempDir, _workingDir
  if args.cont:
    debug('Skipping temporary directory creation due to use of -continue option')
    return
  if _tempDir:
    error('Script error: Cannot use multiple temporary directories')
  if args.tempdir:
    dir_path = os.path.abspath(args.tempdir)
  else:
    if 'ScriptTmpDir' in config:
      dir_path = config['ScriptTmpDir']
    else:
      # Defaulting to working directory since too many users have encountered storage issues
      dir_path = _workingDir
  if 'ScriptTmpPrefix' in config:
    prefix = config['ScriptTmpPrefix']
  else:
    prefix = os.path.basename(sys.argv[0]) + '-tmp-'
  _tempDir = dir_path
  while os.path.isdir(_tempDir):
    random_string = ''.join(random.choice(string.ascii_uppercase + string.digits) for x in range(6))
    _tempDir = os.path.join(dir_path, prefix + random_string) + os.sep
  os.makedirs(_tempDir)
  console('Generated temporary directory: ' + _tempDir)
  with open(os.path.join(_tempDir, 'cwd.txt'), 'w') as outfile:
    outfile.write(_workingDir + '\n')
  with open(os.path.join(_tempDir, 'command.txt'), 'w') as outfile:
    outfile.write(' '.join(sys.argv) + '\n')
  open(os.path.join(_tempDir, 'log.txt'), 'w').close()



def gotoTempDir():
  import os
  global _tempDir
  if not _tempDir:
    error('Script error: No temporary directory location set')
  if _verbosity:
    console('Changing to temporary directory (' + _tempDir + ')')
  os.chdir(_tempDir)



def complete():
  import os, shutil, sys
  global _cleanup, _tempDir, _workingDir
  global colourClear, colourConsole, colourWarn
  console('Changing back to original directory (' + _workingDir + ')')
  os.chdir(_workingDir)
  if _cleanup and _tempDir:
    console('Deleting temporary directory ' + _tempDir)
    shutil.rmtree(_tempDir)
  elif _tempDir:
    # This needs to be printed even if the -quiet option is used
    if os.path.isfile(os.path.join(_tempDir, 'error.txt')):
      with open(os.path.join(_tempDir, 'error.txt'), 'r') as errortext:
        sys.stderr.write(os.path.basename(sys.argv[0]) + ': ' + colourWarn + 'Script failed while executing the command: ' + errortext.readline().rstrip() + colourClear + '\n')
      sys.stderr.write(os.path.basename(sys.argv[0]) + ': ' + colourWarn + 'For debugging, inspect contents of temporary directory: ' + _tempDir + colourClear + '\n')
    else:
      sys.stderr.write(os.path.basename(sys.argv[0]) + ': ' + colourConsole + 'Contents of temporary directory kept, location: ' + _tempDir + colourClear + '\n')
    sys.stderr.flush()





# A set of functions and variables for printing various information at the command-line.
clearLine = ''
colourClear = ''
colourConsole = ''
colourDebug = ''
colourError = ''
colourExec = ''
colourWarn = ''

def console(text):
  import os, sys
  global colourClear, colourConsole
  global _verbosity
  if _verbosity:
    sys.stderr.write(os.path.basename(sys.argv[0]) + ': ' + colourConsole + text + colourClear + '\n')

def debug(text):
  import inspect, os, sys
  global colourClear, colourDebug
  global _verbosity
  if _verbosity <= 2: return
  stack = inspect.stack()[1]
  try:
    fname = stack.function
  except: # Prior to Version 3.5
    fname = stack[3]
  sys.stderr.write(os.path.basename(sys.argv[0]) + ': ' + colourDebug + '[DEBUG] ' + fname + '(): ' + text + colourClear + '\n')

def error(text):
  import os, sys
  global colourClear, colourError
  global _cleanup
  sys.stderr.write(os.path.basename(sys.argv[0]) + ': ' + colourError + '[ERROR] ' + text + colourClear + '\n')
  _cleanup = False
  complete()
  sys.exit(1)

def warn(text):
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

  def __init__(self, *args, **kwargs):
    import sys
    global _defaultCopyright
    if 'author' in kwargs:
      self._author = kwargs['author']
      del kwargs['author']
    else:
      self._author = None
    self._citationList = [ ]
    if 'copyright' in kwargs:
      self._copyright = kwargs['copyright']
      del kwargs['copyright']
    else:
      self._copyright = _defaultCopyright
    self._description = [ ]
    self._externalCitations = False
    if 'synopsis' in kwargs:
      self.synopsis = kwargs['synopsis']
      del kwargs['synopsis']
    else:
      self.synopsis = None
    kwargs['add_help'] = False
    argparse.ArgumentParser.__init__(self, *args, **kwargs)
    self.mutuallyExclusiveOptionGroups = [ ]
    if 'parents' in kwargs:
      for parent in kwargs['parents']:
        self._citationList.extend(parent._citationList)
        self._externalCitations = self._externalCitations or parent._externalCitations
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

  def addCitation(self, condition, reference, is_external):
    self._citationList.append( (condition, reference) )
    if is_external:
      self._externalCitations = True

  def addDescription(self, text):
    self._description.append(text)

  def setCopyright(text):
    self._copyright = text

  # Mutually exclusive options need to be added before the command-line input is parsed
  def flagMutuallyExclusiveOptions(self, options, required=False):
    import sys
    if not type(options) is list or not type(options[0]) is str:
      sys.stderr.write('Script error: Parser.flagMutuallyExclusiveOptions() only accepts a list of strings\n')
      sys.stderr.flush()
      sys.exit(1)
    self.mutuallyExclusiveOptionGroups.append( (options, required) )

  def parse_args(self):
    import sys
    args = argparse.ArgumentParser.parse_args(self)
    self._checkMutuallyExclusiveOptions(args)
    if self._subparsers:
      for alg in self._subparsers._group_actions[0].choices:
        self._subparsers._group_actions[0].choices[alg]._checkMutuallyExclusiveOptions(args)
    return args

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
    if self._citationList:
      console('')
      citation_warning = 'Note that this script makes use of commands / algorithms that have relevant articles for citation'
      if self._externalCitations:
        citation_warning += '; INCLUDING FROM EXTERNAL SOFTWARE PACKAGES'
      citation_warning += '. Please consult the help page (-help option) for more information.'
      console(citation_warning)
      console('')

  # Overloads argparse.ArgumentParser function to give a better error message on failed parsing
  def error(self, text):
    import shlex, sys
    for arg in sys.argv:
      if '-help'.startswith(arg):
        self._printHelp()
        sys.exit(0)
    if self.prog and len(shlex.split(self.prog)) == len(sys.argv): # No arguments provided to subparser
      self._printHelp()
      sys.exit(0)
    usage = self._formatUsage()
    if self._subparsers:
      for alg in self._subparsers._group_actions[0].choices:
        if alg == sys.argv[1]:
          usage = self._subparsers._group_actions[0].choices[alg]._formatUsage()
          continue
    sys.stderr.write('\nError: %s\n' % text)
    sys.stderr.write('Usage: ' + usage + '\n')
    sys.stderr.write('       (Run ' + self.prog + ' -help for more information)\n\n')
    sys.stderr.flush()
    sys.exit(1)

  def _checkMutuallyExclusiveOptions(self, args):
    import sys
    for group in self.mutuallyExclusiveOptionGroups:
      count = 0
      for option in group[0]:
        # Checking its presence is not adequate; by default, argparse adds these members to the namespace
        # Need to test if more than one of these options DIFFERS FROM ITS DEFAULT
        # Will need to loop through actions to find it manually
        if hasattr(args, option):
          for arg in self._actions:
            if arg.dest == option:
              if not getattr(args, option) == arg.default:
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

  def _formatUsage(self):
    import sys
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

  def _printHelp(self):
    import subprocess, textwrap

    def bold(text):
      return ''.join( c + chr(0x08) + c for c in text)

    def underline(text):
      return ''.join( '_' + chr(0x08) + c for c in text)

    w = textwrap.TextWrapper(width=80, initial_indent='     ', subsequent_indent='     ')
    w_arg = textwrap.TextWrapper(width=80, initial_indent='', subsequent_indent='                     ')

    s = '     ' + bold(self.prog) + ': Script using the MRtrix3 Python library\n'
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
    if self._citationList:
      s += '\n'
      s += bold('REFERENCES') + '\n'
      s += '\n'
      for entry in self._citationList:
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
        print (s)
    else:
      print (s)

  def _printFullUsage(self):
    import sys
    print (self.synopsis)
    if self._description:
      if isinstance(self._description, list):
        for line in self._description:
          print (line)
      else:
        print (self._description)
    if self._subparsers and len(sys.argv) == 3:
      for alg in self._subparsers._group_actions[0].choices:
        if alg == sys.argv[1]:
          self._subparsers._group_actions[0].choices[alg]._printFullUsage()
          return
      self.error('Invalid subparser nominated')
    for arg in self._positionals._group_actions:
      # This will need updating if any scripts allow mulitple argument inputs
      print ('ARGUMENT ' + arg.dest + ' 0 0')
      print (arg.help)
    for group in reversed(self._action_groups):
      if group._group_actions and not (len(group._group_actions) == 1 and isinstance(group._group_actions[0], argparse._SubParsersAction)) and not group == self._positionals:
        for option in group._group_actions:
          print ('OPTION ' + '/'.join(option.option_strings) + ' 1 0')
          print (option.help)
          if option.metavar:
            if isinstance(option.metavar, tuple):
              for arg in option.metavar:
                print ('ARGUMENT ' + arg + ' 0 0')
            else:
              print ('ARGUMENT ' + option.metavar + ' 0 0')

  def _printUsageMarkdown(self):
    import os, subprocess, sys
    if self._subparsers and len(sys.argv) == 3:
      for alg in self._subparsers._group_actions[0].choices:
        if alg == sys.argv[-2]:
          self._subparsers._group_actions[0].choices[alg]._printUsageMarkdown()
          return
      self.error('Invalid subcmdline nominated')
    print ('## Synopsis')
    print ('')
    print (self.synopsis)
    print ('')
    print ('## Usage')
    print ('')
    print ('    ' + self._formatUsage())
    print ('')
    if self._subparsers:
      print ('-  *' + self._subparsers._group_actions[0].dest + '*: ' + self._subparsers._group_actions[0].help)
    for arg in self._positionals._group_actions:
      if arg.metavar:
        name = arg.metavar
      else:
        name = arg.dest
      print ('-  *' + name + '*: ' + arg.help)
    print ('')
    if self._description:
      print ('## Description')
      print ('')
      for line in self._description:
        print (line)
        print ('')
    print ('## Options')
    print ('')
    for group in reversed(self._action_groups):
      if group._group_actions and not (len(group._group_actions) == 1 and isinstance(group._group_actions[0], argparse._SubParsersAction)) and not group == self._positionals:
        print ('#### ' + group.title)
        print ('')
        for option in group._group_actions:
          text = '/'.join(option.option_strings)
          if option.metavar:
            text += ' '
            if isinstance(option.metavar, tuple):
              text += ' '.join(option.metavar)
            else:
              text += option.metavar
          print ('+ **-' + text + '**<br>' + option.help)
          print ('')
    if self._citationList:
      print ('## References')
      print ('')
      for ref in self._citationList:
        text = ''
        if ref[0]:
          text += ref[0] + ': '
        text += ref[1]
        print (text)
        print ('')
    print ('---')
    print ('')
    print ('**Author:** ' + self._author)
    print ('')
    print ('**Copyright:** ' + self._copyright)
    print ('')
    if self._subparsers:
      for alg in self._subparsers._group_actions[0].choices:
        subprocess.call ([ sys.executable, os.path.realpath(sys.argv[0]), alg, '__print_usage_markdown__' ])

  def _printUsageRst(self):
    import os, subprocess, sys
    # Need to check here whether it's the documentation for a particular subcmdline that's being requested
    if self._subparsers and len(sys.argv) == 3:
      for alg in self._subparsers._group_actions[0].choices:
        if alg == sys.argv[-2]:
          self._subparsers._group_actions[0].choices[alg]._printUsageRst()
          return
      self.error('Invalid subparser nominated: ' + sys.argv[-2])
    print ('.. _' + self.prog.replace(' ', '_') + ':')
    print ('')
    print (self.prog)
    print ('='*len(self.prog))
    print ('')
    print ('Synopsis')
    print ('--------')
    print ('')
    print (self.synopsis)
    print ('')
    print ('Usage')
    print ('--------')
    print ('')
    print ('::')
    print ('')
    print ('    ' + self._formatUsage())
    print ('')
    if self._subparsers:
      print ('-  *' + self._subparsers._group_actions[0].dest + '*: ' + self._subparsers._group_actions[0].help)
    for arg in self._positionals._group_actions:
      if arg.metavar:
        name = arg.metavar
      else:
        name = arg.dest
      print ('-  *' + name + '*: ' + arg.help.replace('|', '\\|'))
    print ('')
    if self._description:
      print ('Description')
      print ('-----------')
      print ('')
      for line in self._description:
        print (line)
        print ('')
    print ('Options')
    print ('-------')
    for group in reversed(self._action_groups):
      if group._group_actions and not (len(group._group_actions) == 1 and isinstance(group._group_actions[0], argparse._SubParsersAction)) and not group == self._positionals:
        print ('')
        print (group.title)
        print ('^'*len(group.title))
        for option in group._group_actions:
          text = '/'.join(option.option_strings)
          if option.metavar:
            text += ' '
            if isinstance(option.metavar, tuple):
              text += ' '.join(option.metavar)
            else:
              text += option.metavar
          print ('')
          print ('- **' + text + '** ' + option.help.replace('|', '\\|'))
    if self._citationList:
      print ('')
      print ('References')
      print ('^^^^^^^^^^')
      for ref in self._citationList:
        text = '* '
        if ref[0]:
          text += ref[0] + ': '
        text += ref[1]
        print ('')
        print (text)
    print ('')
    print ('--------------')
    print ('')
    print ('')
    print ('')
    print ('**Author:** ' + self._author)
    print ('')
    print ('**Copyright:** ' + self._copyright)
    print ('')
    if self._subparsers:
      sys.stdout.flush()
      for alg in self._subparsers._group_actions[0].choices:
        subprocess.call ([ sys.executable, os.path.realpath(sys.argv[0]), alg, '__print_usage_rst__' ])



# A class that can be used to display a progress bar on the terminal,
#   mimicing the behaviour of MRtrix3 binary commands
class progressBar:

  def _update(self):
    global clearLine, colourConsole, colourClear
    sys.stderr.write('\r' + colourConsole + os.path.basename(sys.argv[0]) + ': ' + colourClear + '[{0:>3}%] '.format(int(round(100.0*self.counter/self.target))) + self.message + '...' + clearLine + self.newline)
    sys.stderr.flush()

  def __init__(self, msg, target):
    global _verbosity
    self.counter = 0
    self.message = msg
    self.newline = '\n' if _verbosity > 1 else '' # If any more than default verbosity, may still get details printed in between progress updates
    self.orig_verbosity = _verbosity
    self.target = target
    _verbosity = _verbosity - 1 if _verbosity else 0
    self._update()

  def increment(self, msg=''):
    if msg:
      self.message = msg
    self.counter += 1
    self._update()

  def done(self):
    global _verbosity
    global clearLine, colourConsole, colourClear
    self.counter = self.target
    sys.stderr.write('\r' + colourConsole + os.path.basename(sys.argv[0]) + ': ' + colourClear + '[100%] ' + self.message + clearLine + '\n')
    sys.stderr.flush()
    _verbosity = self.orig_verbosity




# Return a boolean flag to indicate whether or not script is being run on a Windows machine
def isWindows():
  import platform
  system = platform.system().lower()
  return system.startswith('mingw') or system.startswith('msys') or system.startswith('windows')




# Handler function for dealing with system signals
def _handler(signum, frame):
  import os, signal, sys
  global _signals
  # First, kill any child processes
  try:
    from mrtrix3.run import _processes
    for p in _processes:
      if p:
        p.terminate()
    _processes = [ ]
  except ImportError:
    pass
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
  sys.stderr.write(os.path.basename(sys.argv[0]) + ': ' + colourError + msg + colourClear + '\n')
  complete()
  exit(signum)

