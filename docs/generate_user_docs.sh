#!/bin/bash

#
# Generate .rst documentation for commands/scripts listing
# Based off of the update_doc script originally used to update the MRtrix wiki
#
# Author: Rami Tabbara
#



# Generating documentation for all commands

mrtrix_root=$( cd "$(dirname "${BASH_SOURCE}")"/../ ; pwd -P )
export PATH=$mrtrix_root/release/bin:$mrtrix_root/release/scripts:"$PATH"

  echo "
################
List of MRtrix3 commands
################


.. toctree::
   :maxdepth: 1

" > reference/commands_list.rst

  mkdir -p reference/commands
  for n in `find ../cmd/ -name "*.cpp" | sort`; do
    dirpath='reference/commands'
    cppname=`basename $n`
    cmdname=${cppname%".cpp"}
    cmdpath=$cmdname
    if [ "$OSTYPE" == "cygwin" ] || [ "$OSTYPE" == "msys" ] || [ "$OSTYPE" == "win32" ]; then
      cmdpath=${cmdpath}'.exe'
    fi
    $cmdpath __print_usage_rst__ > $dirpath/$cmdname.rst
    sed -ie "1i.. _$cmdname:\n\n$cmdname\n===========\n" $dirpath/$cmdname.rst
    echo '
   commands/'"$cmdname" >> reference/commands_list.rst
  done

# Generating documentation for all scripts

  echo "
################
Python scripts provided with MRtrix3
################


.. toctree::
   :maxdepth: 1

" > reference/scripts_list.rst

  mkdir -p reference/scripts
  for n in `find ../scripts/ -type f -print0 | xargs -0 grep -l "lib.cmdlineParser.initialise" | sort`; do
    filepath='reference/scripts'
    filename=`basename $n`
    $n __print_usage_rst__ > $filepath/$filename.rst
    sed -ie "1i$filename\n===========\n" $filepath/$filename.rst
    echo '
   scripts/'"$filename" >> reference/scripts_list.rst
  done

# Generating list of configuration file options

  grep -rn --include=\*.h --include=\*.cpp '^\s*//CONF\b ' ../ | sed -ne 's/^.*CONF \(.*\)/\1/p' | ./format_config_options > reference/config_file_options.rst

