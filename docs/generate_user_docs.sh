#!/bin/bash

#
# Generate .rst documentation for commands/scripts listing
# Based off of the update_doc script originally used to update the MRtrix wiki
#
# Author: Rami Tabbara
#



# Generating documentation for all commands

mrtrix_root=$( cd "$(dirname "${BASH_SOURCE}")"/../ ; pwd -P )
export PATH=$mrtrix_root/bin:"$PATH"

  echo "
.. _list-of-mrtrix3-commands:

########################
List of MRtrix3 commands
########################

" > reference/commands_list.rst

  mkdir -p reference/commands
  toctree_file=$(mktemp)
  table_file=$(mktemp)

  echo "

.. toctree::
    :hidden:
" > $toctree_file

  echo "

.. csv-table::
    :header: \"Command\", \"Synopsis\"
" > $table_file

  for n in `find ../cmd/ -name "*.cpp" | sort`; do
    dirpath='reference/commands'
    cppname=`basename $n`
    cmdname=${cppname%".cpp"}
    cmdpath=$cmdname
    if [ "$OSTYPE" == "cygwin" ] || [ "$OSTYPE" == "msys" ] || [ "$OSTYPE" == "win32" ]; then
      cmdpath=${cmdpath}'.exe'
    fi
    $cmdpath __print_usage_rst__ > $dirpath/$cmdname.rst
    sed -ie "1i.. _$cmdname:\n\n$cmdname\n===================\n" $dirpath/$cmdname.rst
    echo '    commands/'"$cmdname" >> $toctree_file
    echo '    :ref:`'"$cmdname"'`, "'`$cmdpath __print_synopsis__`'"' >> $table_file
  done
  cat $toctree_file $table_file >> reference/commands_list.rst
  rm -f $toctree_file $temp_file

# Generating documentation for all scripts

  echo "
.. _list-of-mrtrix3-scripts:

#######################
List of MRtrix3 scripts
#######################

" > reference/scripts_list.rst

  mkdir -p reference/scripts
  toctree_file=$(mktemp)
  table_file=$(mktemp)

  echo "

.. toctree::
    :hidden:
" > $toctree_file

  echo "

.. csv-table::
    :header: \"Command\", \"Synopsis\"
" > $table_file

  for n in `find ../bin/ -type f -print0 | xargs -0 grep -l "app.parse" | sort`; do
    filepath='reference/scripts'
    filename=`basename $n`
    $n __print_usage_rst__ > $filepath/$filename.rst
    #sed -ie "1i$filename\n===========\n" $filepath/$filename.rst

    echo '    scripts/'"$filename" >> $toctree_file
    echo '    :ref:`'"$filename"'`, "'`$filename __print_synopsis__`'"' >> $table_file
  done
  cat $toctree_file $table_file >> reference/scripts_list.rst
  rm -f $toctree_file $temp_file

# Generating list of configuration file options

  grep -rn --include=\*.h --include=\*.cpp '^\s*//CONF\b ' ../ | sed -ne 's/^.*CONF \(.*\)/\1/p' | ./format_config_options > reference/config_file_options.rst

