#!/bin/bash

#
# Generate .rst documentation for commands/scripts listing
# Based off of the update_doc script originally used to update the MRtrix wiki
#
# Author: Rami Tabbara
#



# Generating documentation for all commands

  echo "
################
List of MRtrix3 commands
################" > reference/commands_list.rst

  mkdir -p reference/commands
  for n in `find ../cmd/ -name "*.cpp" | sort`; do
    dirpath='reference/commands'
    cmdname=${n##*/}
    cmdname=${cmdname%".cpp"}
    cmdpath=$cmdname
    if [ "$OSTYPE" == "cygwin" ] || [ "$OSTYPE" == "msys" ] || [ "$OSTYPE" == "win32" ]; then
      cmdpath=${cmdpath}".exe"
    fi
    $cmdpath __print_usage_rst__ > $dirpath/$cmdname.rst
    sed -ie "1i.. _$cmdname:\n\n$cmdname\n===========\n" $dirpath/$cmdname.rst
    echo "
.. include:: commands/$cmdname.rst
.......
" >> reference/commands_list.rst
  done

# Generating documentation for all scripts

  echo "
################
Python scripts provided with MRtrix3
################" > reference/scripts_list.rst

  mkdir -p reference/scripts
  for n in `find ../scripts/ -type f -print0 | xargs -0 grep -l "lib.app.initParser" | sort`; do
    filepath='reference/scripts'
    filename=`basename $n`
    $n __print_usage_rst__ > $filepath/$filename.rst
    sed -ie "1i$filename\n===========\n" $filepath/$filename.rst
    echo "
.. include:: scripts/$filename.rst
.......
" >> reference/scripts_list.rst
  done
  
