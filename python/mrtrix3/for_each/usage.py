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

import sys
from . import CMDSPLIT

def usage(cmdline): #pylint: disable=unused-variable
  global CMDSPLIT
  from mrtrix3 import _version #pylint: disable=no-name-in-module, import-outside-toplevel
  cmdline.set_author('Robert E. Smith (robert.smith@florey.edu.au) and David Raffelt (david.raffelt@florey.edu.au)')
  cmdline.set_synopsis('Perform some arbitrary processing step for each of a set of inputs')
  cmdline.add_description('This script greatly simplifies various forms of batch processing by enabling the execution of a command (or set of commands) independently for each of a set of inputs.')
  cmdline.add_description('More information on use of the for_each command can be found at the following link: \n'
                          'https://mrtrix.readthedocs.io/en/' + _version.__tag__ + '/tips_and_tricks/batch_processing_with_foreach.html')
  cmdline.add_description('The way that this batch processing capability is achieved is by providing basic text substitutions, which simplify the formation of valid command strings based on the unique components of the input strings on which the script is instructed to execute. This does however mean that the items to be passed as inputs to the for_each command (e.g. file / directory names) MUST NOT contain any instances of these substitution strings, as otherwise those paths will be corrupted during the course of the substitution.')
  cmdline.add_description('The available substitutions are listed below (note that the -test command-line option can be used to ensure correct command string formation prior to actually executing the commands):')
  cmdline.add_description('   - IN:   The full matching pattern, including leading folders. For example, if the target list contains a file "folder/image.mif", any occurrence of "IN" will be substituted with "folder/image.mif".')
  cmdline.add_description('   - NAME: The basename of the matching pattern. For example, if the target list contains a file "folder/image.mif", any occurrence of "NAME" will be substituted with "image.mif".')
  cmdline.add_description('   - PRE:  The prefix of the input pattern (the basename stripped of its extension). For example, if the target list contains a file "folder/my.image.mif.gz", any occurrence of "PRE" will be substituted with "my.image".')
  cmdline.add_description('   - UNI:  The unique part of the input after removing any common prefix and common suffix. For example, if the target list contains files: "folder/001dwi.mif", "folder/002dwi.mif", "folder/003dwi.mif", any occurrence of "UNI" will be substituted with "001", "002", "003".')
  cmdline.add_description('Note that due to a limitation of the Python "argparse" module, any command-line OPTIONS that the user intends to provide specifically to the for_each script must appear BEFORE providing the list of inputs on which for_each is intended to operate. While command-line options provided as such will be interpreted specifically by the for_each script, any command-line options that are provided AFTER the COLON separator will form part of the executed COMMAND, and will therefore be interpreted as command-line options having been provided to that underlying command.')
  cmdline.add_example_usage('Demonstration of basic usage syntax',
                            'for_each folder/*.mif : mrinfo IN',
                            'This will run the "mrinfo" command for every .mif file present in "folder/". Note that the compulsory colon symbol is used to separate the list of items on which for_each is being instructed to operate, from the command that is intended to be run for each input.')
  cmdline.add_example_usage('Multi-threaded use of for_each',
                            'for_each -nthreads 4 freesurfer/subjects/* : recon-all -subjid NAME -all',
                            'In this example, for_each is instructed to run the FreeSurfer command \'recon-all\' for all subjects within the \'subjects\' directory, with four subjects being processed in parallel at any one time. Whenever processing of one subject is completed, processing for a new unprocessed subject will commence. This technique is useful for improving the efficiency of running single-threaded commands on multi-core systems, as long as the system possesses enough memory to support such parallel processing. Note that in the case of multi-threaded commands (which includes many MRtrix3 commands), it is generally preferable to permit multi-threaded execution of the command on a single input at a time, rather than processing multiple inputs in parallel.')
  cmdline.add_example_usage('Excluding specific inputs from execution',
                            'for_each *.nii -exclude 001.nii : mrconvert IN PRE.mif',
                            'Particularly when a wildcard is used to define the list of inputs for for_each, it is possible in some instances that this list will include one or more strings for which execution should in fact not be performed; for instance, if a command has already been executed for one or more files, and then for_each is being used to execute the same command for all other files. In this case, the -exclude option can be used to effectively remove an item from the list of inputs that would otherwise be included due to the use of a wildcard (and can be used more than once to exclude more than one string). In this particular example, mrconvert is instructed to perform conversions from NIfTI to MRtrix image formats, for all except the first image in the directory. Note that any usages of this option must appear AFTER the list of inputs. Note also that the argument following the -exclude option can alternatively be a regular expression, in which case any inputs for which a match to the expression is found will be excluded from processing.')
  cmdline.add_example_usage('Testing the command string substitution',
                            'for_each -test * : mrconvert IN PRE.mif',
                            'By specifying the -test option, the script will print to the terminal the results of text substitutions for all of the specified inputs, but will not actually execute those commands. It can therefore be used to verify that the script is receiving the intended set of inputs, and that the text substitutions on those inputs lead to the intended command strings.')
  cmdline.add_argument('inputs',   help='Each of the inputs for which processing should be run', nargs='+')
  cmdline.add_argument('colon',    help='Colon symbol (":") delimiting the for_each inputs & command-line options from the actual command to be executed', type=str, choices=[':'])
  cmdline.add_argument('command',  help='The command string to run for each input, containing any number of substitutions listed in the Description section', type=str)
  cmdline.add_argument('-exclude', help='Exclude one specific input string / all strings matching a regular expression from being processed (see Example Usage)', action='append', metavar='"regex"', nargs=1)
  cmdline.add_argument('-test',    help='Test the operation of the for_each script, by printing the command strings following string substitution but not actually executing them', action='store_true', default=False)

  # Usage of for_each needs to be handled slightly differently here:
  # We want argparse to parse only the contents of the command-line before the colon symbol,
  #   as these are the items that pertain to the invocation of the for_each script;
  #   anything after the colon should instead form a part of the command that
  #   for_each is responsible for executing
  try:
    index = next(i for i,s in enumerate(sys.argv) if s == ':')
    try:
      CMDSPLIT = sys.argv[index+1:]
      sys.argv = sys.argv[:index+1]
      sys.argv.append(' '.join(CMDSPLIT))
    except IndexError:
      sys.stderr.write('Erroneous usage: No command specified (colon separator cannot be the last entry provided)\n')
      sys.exit(0)
  except StopIteration:
    if len(sys.argv) > 2:
      sys.stderr.write('Erroneous usage: A colon must be used to separate for_each inputs from the command to be executed\n')
      sys.exit(0)
