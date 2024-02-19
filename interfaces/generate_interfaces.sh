#!/bin/bash

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


mrtrix_root=$( cd "$(dirname "${BASH_SOURCE}")"/../ ; pwd -P )
export PATH=$mrtrix_root/bin:"$PATH"
export LC_ALL=C

cmdlist=""
for n in `find "${mrtrix_root}"/cmd/ -name "*.cpp"`; do
  cmdlist=$cmdlist$'\n'`basename $n`
done
for n in `find "${mrtrix_root}"/bin/ -type f -print0 | xargs -0 grep -l "import mrtrix3"`; do
  cmdlist=$cmdlist$'\n'`basename $n`
done

legacy_root="${mrtrix_root}/interfaces/legacy"
rm -rf "${legacy_root}"
mkdir "${legacy_root}"
for n in `echo "$cmdlist" | sort`; do
  cmdname=${n%".cpp"}
  cmdpath=$cmdname
  case $n in *.cpp)
    if [ "$OSTYPE" == "cygwin" ] || [ "$OSTYPE" == "msys" ] || [ "$OSTYPE" == "win32" ]; then
      cmdpath=${cmdpath}'.exe'
    fi
  esac
  $cmdpath __print_full_usage__ > $legacy_root/$cmdname.txt
  if `grep -q "algorithm\.get_module" "${mrtrix_root}/bin/${cmdpath}"`; then
    mkdir $legacy_root/$cmdname
    algorithms=`grep "ARGUMENT algorithm CHOICE" $legacy_root/$cmdname.txt | cut -d" " -f 4-`
    for a in $algorithms; do
      $cmdpath $a __print_full_usage__ > $legacy_root/$cmdname/$a.txt
    done
  fi
done
