# Set of functionalities for when a single script has many 'algorithms' that may be invoked,
#   i.e. the script deals with generating a particular output, but there are a number of
#   processes to select from, each of which is capable of generating that output.



# Helper function for finding where the files representing different script algorithms will be stored
# These will be in a sub-directory relative to this library file
def _algorithmsPath():
  import os
  from mrtrix3 import path
  return os.path.join(os.path.dirname(__file__), path.scriptSubDirName())



# This function needs to be safe to run in order to populate the help page; that is, no app initialisation has been run
def getList():
  import os, sys
  from mrtrix3 import app
  global _algorithm_list
  algorithm_list = [ ]
  src_file_list = os.listdir(_algorithmsPath())
  for filename in src_file_list:
    filename = filename.split('.')
    if len(filename) == 2 and filename[1] == 'py' and not filename[0] == '__init__':
      algorithm_list.append(filename[0])
  algorithm_list = sorted(algorithm_list)
  return algorithm_list



# Note: This function essentially duplicates the current state of app.cmdline in order for command-line
#   options common to all algorithms of a particular script to be applicable once any particular sub-parser
#   is invoked. Therefore this function must be called _after_ all such options are set up.
def initialise():
  import importlib, pkgutil
  from mrtrix3 import app, path
  initlist = [ ]
  base_parser = app.Parser(description='Base parser for construction of subparsers', parents=[app.cmdline])
  subparsers = app.cmdline.add_subparsers(title='Algorithm choices', help='Select the algorithm to be used to complete the script operation; additional details and options become available once an algorithm is nominated. Options are: ' + ', '.join(getList()), dest='algorithm')
  for importer, package_name, ispkg in pkgutil.iter_modules( [ _algorithmsPath() ] ):
    module = importlib.import_module('mrtrix3.' + path.scriptSubDirName() + '.' + package_name)
    module.initialise(base_parser, subparsers)
    initlist.extend(package_name)




def getModule(name):
  import sys
  from mrtrix3 import path
  return sys.modules['mrtrix3.' + path.scriptSubDirName() + '.' + name]

