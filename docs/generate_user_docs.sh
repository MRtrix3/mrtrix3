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
################" > getting_started/commands_list.rst

  mkdir -p getting_started/commands
  for n in ../release/bin/*; do
    filepath='getting_started/commands'
    filename=`basename $n`
    $n __print_usage_rst__ > $filepath/$filename.rst
    sed -ie "1i.. _$filename:\n\n$filename\n===========\n" $filepath/$filename.rst
    echo "
.. include:: commands/$filename.rst
.......
" >> getting_started/commands_list.rst
  done

# Generating documentation for all scripts

  echo "
################
Scripts for external libraries
################" > getting_started/scripts_list.rst

  mkdir -p getting_started/scripts
  for n in `find ../scripts/ -type f -print0 | xargs -0 grep -l "lib.app.initParser" | sort`; do
    filepath='getting_started/scripts'
    filename=`basename $n`
    $n __print_usage_rst__ > $filepath/$filename.rst
    sed -ie "1i$filename\n===========\n" $filepath/$filename.rst
    echo "
.. include:: scripts/$filename.rst
.......
" >> getting_started/scripts_list.rst
  done
  
