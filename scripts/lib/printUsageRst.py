def printUsageRst (args, stdopts, refs, author):
  import argparse, os, sys
  
  argument_list = [ ]
  for arg in args._positionals._group_actions:
    argument_list.append(arg.dest)
  
  standard_option_list = [ ]
  for opt in stdopts._group_actions:
    standard_option_list.append(opt.option_strings[0])
    
  help_option = ''
  option_list = [ ]
  for opt in args._actions:
    if opt.option_strings and not opt.option_strings[0] in argument_list and not opt.option_strings[0] in standard_option_list:
      if opt.option_strings[0] == '-h':
        help_option = opt
      else:
        option_list.append(opt.option_strings[0])
  
  print ('Synopsis')
  print ('--------')
  print ('')
  print ('    ' + args.prog + ' [ options ] ' + ' '.join(argument_list))
  print ('')
  
  for arg in args._positionals._group_actions:
    _printArgument(arg)
  
  print ('')
  print ('Description')
  print ('-----------')
  print ('')
  print (vars(args)['description'])
  print ('')
  
  if option_list:
    print ('Options')
    print ('-------')
    print ('')
  
    for opt in args._actions:
      if opt.option_strings and opt.option_strings[0] in option_list:
        _printOption(opt)
        print ('')
  
  print ('Standard options')
  print ('^^^^^^^^^^^^^^^^')
  print ('')
  
  _printOption(help_option)
  print ('')
  
  for opt in stdopts._group_actions:
    _printOption(opt)
    print ('')
      
  print ('References')
  print ('^^^^^^^^^^')
  print ('')
  print (refs)
  print ('')
  print ('---')
  print ('')
  print ('**Author:** ' + author)
  print ('')
  print ('''**Copyright:** 
Copyright (c) 2008-2016 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public 
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/

MRtrix is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see www.mrtrix.org''')
  
  
  
  
def _printArgument(arg):
  print ('- *' + arg.dest + '*: ' + arg.help)
  
  
  
def _printOption(opt):
  if not hasattr(opt, 'option_strings'):
    return
  usage = ''
  if len(opt.option_strings) == 1:
    usage = [ opt.option_strings[0] ]
  else:
    usage = [ '[ ' + ' '.join(opt.option_strings) + ' ]' ]
  if type(opt.metavar) is tuple:
    for arg in opt.metavar:
      usage.append(arg)
  elif type(opt.metavar) is str:
    usage.append(opt.metavar)
  print ('- **' + ' '.join(usage) + '** ' + opt.help)
  
