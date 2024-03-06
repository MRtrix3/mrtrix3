# Copyright (c) 2008-2023 the MRtrix3 contributors.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Covered Software is provided under this License on an "as is"
# basis, without warranty of any kind, either expressed, implied, or
# statutory, including, without limitation, warranties that the
# Covered Software is free of defects, merchantable, fit for a
# particular purpose or non-infringing.
# See the Mozilla Public License v. 2.0 for more details.
#
# For more details, see http://www.mrtrix.org/.

# Set of functionalities for when a single script has many 'algorithms' that may be invoked,
#   i.e. the script deals with generating a particular output, but there are a number of
#   processes to select from, each of which is capable of generating that output.


import importlib, inspect, pkgutil, sys



# Note: This function essentially duplicates the current state of app.cmdline in order for command-line
#   options common to all algorithms of a particular script to be applicable once any particular sub-parser
#   is invoked. Therefore this function must be called _after_ all such options are set up.
def usage(cmdline): #pylint: disable=unused-variable
  from mrtrix3 import app #pylint: disable=import-outside-toplevel
  module_name = inspect.currentframe().f_back.f_globals["__name__"]
  submodules = [submodule_info.name for submodule_info in pkgutil.walk_packages(sys.modules[module_name].__spec__.submodule_search_locations)]
  base_parser = app.Parser(description='Base parser for construction of subparsers', parents=[cmdline])
  subparsers = cmdline.add_subparsers(title='Algorithm choices',
                                      help='Select the algorithm to be used to complete the script operation; '
                                           'additional details and options become available once an algorithm is nominated. '
                                           'Options are: ' + ', '.join(submodules), dest='algorithm')
  for submodule in submodules:
    module = importlib.import_module(module_name + '.' + submodule)
    module.usage(base_parser, subparsers)
  return



def get(name): #pylint: disable=unused-variable
  return sys.modules[inspect.currentframe().f_back.f_globals["__name__"] + '.' + name]
