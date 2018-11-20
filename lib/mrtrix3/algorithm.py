# Set of functionalities for when a single script has many 'algorithms' that may be invoked,
#   i.e. the script deals with generating a particular output, but there are a number of
#   processes to select from, each of which is capable of generating that output.



# Helper function for finding where the files representing different script algorithms will be stored
# These will be in a sub-directory relative to this library file
def _algorithms_path():
  import os
  from mrtrix3 import path
  return os.path.join(os.path.dirname(__file__), path.script_subdir_name())



# This function needs to be safe to run in order to populate the help page; that is, no app initialisation has been run
def get_list(): #pylint: disable=unused-variable
  import os
  from mrtrix3 import app
  algorithm_list = [ ]
  for filename in os.listdir(_algorithms_path()):
    filename = filename.split('.')
    if len(filename) == 2 and filename[1] == 'py' and not filename[0] == '__init__':
      algorithm_list.append(filename[0])
  algorithm_list = sorted(algorithm_list)
  app.debug('Found algorithms: ' + str(algorithm_list))
  return algorithm_list



# Note: This function essentially duplicates the current state of app.cmdline in order for command-line
#   options common to all algorithms of a particular script to be applicable once any particular sub-parser
#   is invoked. Therefore this function must be called _after_ all such options are set up.
def usage(cmdline): #pylint: disable=unused-variable
  import importlib, pkgutil
  from mrtrix3 import app, path
  initlist = [ ]
  base_parser = app.Parser(description='Base parser for construction of subparsers', parents=[cmdline])
  subparsers = cmdline.add_subparsers(title='Algorithm choices', help='Select the algorithm to be used to complete the script operation; additional details and options become available once an algorithm is nominated. Options are: ' + ', '.join(get_list()), dest='algorithm')
  for dummy_importer, package_name, dummy_ispkg in pkgutil.iter_modules( [ _algorithms_path() ] ):
    module = importlib.import_module('mrtrix3.' + path.script_subdir_name() + '.' + package_name)
    module.usage(base_parser, subparsers)
    initlist.extend(package_name)
  app.debug('Initialised algorithms: ' + str(initlist))



def get_module(name): #pylint: disable=unused-variable
  import sys
  from mrtrix3 import path
  return sys.modules['mrtrix3.' + path.script_subdir_name() + '.' + name]
