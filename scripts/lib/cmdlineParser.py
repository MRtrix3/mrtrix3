
import argparse


def initialise(desc):
  import argparse
  import lib.app
  from lib.errorMessage import errorMessage
  global parser
  if not lib.app.author:
    errorMessage('Script error: No author defined')
  lib.app.parser = Parser(description=desc, add_help=False)
  standard_options = lib.app.parser.add_argument_group('Standard options')
  standard_options.add_argument('-continue', nargs=2, dest='cont', metavar=('<TempDir>', '<LastFile>'), help='Continue the script from a previous execution; must provide the temporary directory path, and the name of the last successfully-generated file')
  standard_options.add_argument('-force', action='store_true', help='Force overwrite of output files if pre-existing')
  standard_options.add_argument('-help', action='store_true', help='Display help information for the script')
  standard_options.add_argument('-nocleanup', action='store_true', help='Do not delete temporary files during script, or temporary directory at script completion')
  standard_options.add_argument('-nthreads', metavar='number', help='Use this number of threads in MRtrix multi-threaded applications (0 disables multi-threading)')
  standard_options.add_argument('-tempdir', metavar='/path/to/tmp/', help='Manually specify the path in which to generate the temporary directory')
  verbosity_group = standard_options.add_mutually_exclusive_group()
  verbosity_group.add_argument('-quiet',   action='store_true', help='Suppress all console output during script execution')
  verbosity_group.add_argument('-verbose', action='store_true', help='Display additional information for every command invoked')



class Parser(argparse.ArgumentParser):

  def error(self, message):
    import sys
    import lib.app
    for arg in sys.argv:
      if '-help'.startswith(arg):
        self.print_help()
        sys.exit(0)
    sys.stderr.write('\nError: %s\n' % message)
    sys.stderr.write('Usage: ' + self.format_usage() + '\n')
    sys.stderr.write('Usage: (Run ' + self.prog + ' -help for more information)')
    sys.exit(2)



  def print_help(self):
    global author, citationList, copyright
    import subprocess
    import lib.app
    from lib.readMRtrixConfSetting import readMRtrixConfSetting
    
    def bold(text):
      return ''.join( c + chr(0x08) + c for c in text)
    
    def underline(text):
      return ''.join( '_' + chr(0x08) + c for c in text)
    
    s = '     ' + bold(self.prog) + ': Script using the MRtrix3 Python libraries\n' 
    s += '\n'
    s += bold('SYNOPSIS') + '\n'
    s += '\n'
    s += '     ' + underline(self.prog) + ' [ options ]'
    # Compulsory sub-parser algorithm selection (if present)
    if self._subparsers:
      s += ' ' + self._subparsers._group_actions[0].dest + ' ...'
    # Find compulsory input arguments
    for arg in self._positionals._group_actions:
      if arg.metavar:
        s += ' ' + arg.metavar
      else:
        s += ' ' + arg.dest
    s += '\n'
    s += '\n'
    if self._subparsers:
      s += '        ' + underline(self._subparsers._group_actions[0].dest) + '    ' + self._subparsers._group_actions[0].help + '\n'
      s += '\n'
    for arg in self._positionals._group_actions:
      s += '        '
      if arg.metavar:
        s += underline(arg.metavar)
      else:
        s += underline(arg.dest)
      s += '    ' + arg.help + '\n'
      s += '\n'
    s += bold('DESCRIPTION') + '\n'
    s += '\n'
    s += '     ' + self.description + '\n'
    s += '\n'
    # Option groups
    for group in reversed(self._action_groups):
      # * Don't display empty groups
      # * Don't display the subparser option; that's dealt with in the synopsis
      # * Don't re-display any compulsory positional arguments; they're also dealt with in the synopsis
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
          s += '\n'
          s += '     ' + option.help + '\n'
          s += '\n'
    s += bold('AUTHOR') + '\n'
    s += '     ' + lib.app.author + '\n'
    s += '\n'
    s += bold('COPYRIGHT') + '\n'
    s += lib.app.copyright + '\n'
    if lib.app.citationList:
      s += '\n'
      s += bold('REFERENCES') + '\n'
      s += '\n'
      for entry in lib.app.citationList:
        if entry[0]:
          s += '     * ' + entry[0] + ':\n'
        s += '     ' + entry[1] + '\n'
        s += '\n'
    command = readMRtrixConfSetting('HelpCommand')
    if not command:
      command = 'less -X'
    if command:
      try:
        process = subprocess.Popen(command.split(' '), stdin=subprocess.PIPE)
        process.communicate(s.encode())
      except:
        print (s)
    else:
      print (s)



  def format_usage(self):
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
    return self.prog + ' [ options ] ' + ' '.join(argument_list) + trailing_ellipsis



  def printUsageRst(self):
    import subprocess, sys
    import lib.app
    from lib.getAlgorithmList import getAlgorithmList
    global author, citationList, copyright
    # Need to check here whether it's the documentation for a particular subparser that's being requested
    if self._subparsers and len(sys.argv) == 3:
      for alg in self._subparsers._group_actions[0].choices:
        if alg == sys.argv[1]:
          self._subparsers._group_actions[0].choices[alg].printUsageRst()
          return
      self.error('Invalid subparser nominated')
    print ('.. _' + self.prog.replace(' ', '_') + ':')
    print ('')
    print (self.prog)
    print ('='*len(self.prog))
    print ('')
    print ('Synopsis')
    print ('--------')
    print ('')
    print ('::')
    print ('')
    print ('    ' + self.format_usage())
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
    print ('Description')
    print ('-----------')
    print ('')
    print (self.description)
    print ('')
    print ('Options')
    print ('-------')
    for group in reversed(self._action_groups):
      if group._group_actions and not (len(group._group_actions) == 1 and isinstance(group._group_actions[0], argparse._SubParsersAction)) and not group == self._positionals:
        print ('')
        print (group.title)
        print ('='*len(group.title))
        for option in group._group_actions:
          text = '/'.join(option.option_strings)
          if option.metavar:
            text += ' '
            if isinstance(option.metavar, tuple):
              text += ' '.join(option.metavar)
            else:
              text += option.metavar
          print ('')
          print ('-  **' + text + '** ' + option.help)
    if lib.app.citationList:
      print ('')
      print ('References')
      print ('^^^^^^^^^^')
      for ref in lib.app.citationList:
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
    print ('**Author:** ' + lib.app.author)
    print ('')
    print ('**Copyright:** ' + lib.app.copyright)
    print ('')
    if self._subparsers:
      for alg in getAlgorithmList():
        proc = subprocess.call ([ self.prog, alg, '__print_usage_rst__' ])

