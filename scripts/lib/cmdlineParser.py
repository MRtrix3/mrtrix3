
import argparse


mutuallyExclusiveOptionGroups = [ ]


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
  standard_options.add_argument('-quiet',   action='store_true', help='Suppress all console output during script execution')
  standard_options.add_argument('-verbose', action='store_true', help='Display additional information for every command invoked')
  flagMutuallyExclusiveOptions( [ 'quiet', 'verbose' ] )



def flagMutuallyExclusiveOptions(options, required=False):
  import sys
  if not type(options) is list or not type(options[0]) is str:
    sys.stderr.write('Script error: cmdlineParser.flagMutuallyExclusiveOptions() only accepts a list of strings\n')
    sys.exit(1)
  mutuallyExclusiveOptionGroups.append( (options, required) )



class Parser(argparse.ArgumentParser):

  def error(self, message):
    import shlex, sys
    import lib.app
    for arg in sys.argv:
      if '-help'.startswith(arg):
        self.print_help()
        sys.exit(0)
    if len(shlex.split(self.prog)) == len(sys.argv): # No arguments provided to subparser
      self.print_help()
      sys.exit(0)
    sys.stderr.write('\nError: %s\n' % message)
    sys.stderr.write('Usage: ' + self.format_usage() + '\n')
    sys.stderr.write('Usage: (Run ' + self.prog + ' -help for more information)\n\n')
    sys.exit(2)



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



  def parse_args(self):
    import sys
    args = argparse.ArgumentParser.parse_args(self)
    for group in mutuallyExclusiveOptionGroups:
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
        sys.stderr.write('\nError: You cannot use more than one of the following options: ' + ', '.join([ '-' + o for o in group[0] ]) + '\n\n')
        sys.exit(2)
      if group[1] and not count:
        sys.stderr.write('\nError: One of the following options must be provided: ' + ', '.join([ '-' + o for o in group[0] ]) + '\n\n')
    return args



  def print_help(self):
    import subprocess, textwrap
    import lib.app
    from lib.readMRtrixConfSetting import readMRtrixConfSetting
    global author, citationList, copyright

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
    synopsis = self.prog + ' [ options ]'
    # Compulsory sub-parser algorithm selection (if present)
    if self._subparsers:
      synopsis += ' ' + self._subparsers._group_actions[0].dest + ' ...'
    # Find compulsory input arguments
    for arg in self._positionals._group_actions:
      if arg.metavar:
        synopsis += ' ' + arg.metavar
      else:
        synopsis += ' ' + arg.dest
    # Unfortunately this can line wrap early because textwrap is counting each
    #   underlined character as 3 characters when calculating when to wrap
    # Fix by underlining after the fact
    s += w.fill(synopsis).replace(self.prog, underline(self.prog), 1) + '\n'
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
    s += bold('DESCRIPTION') + '\n'
    s += '\n'
    s += w.fill(self.description) + '\n'
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
          elif option.nargs:
            s += (' ' + option.dest.upper())*option.nargs
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
    s += w.fill(lib.app.author) + '\n'
    s += '\n'
    s += bold('COPYRIGHT') + '\n'
    s += w.fill(lib.app.copyright) + '\n'
    if lib.app.citationList:
      s += '\n'
      s += bold('REFERENCES') + '\n'
      s += '\n'
      for entry in lib.app.citationList:
        if entry[0]:
          s += w.fill('* ' + entry[0] + ':') + '\n'
        s += w.fill(entry[1]) + '\n'
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
          print ('- **' + text + '** ' + option.help)
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

