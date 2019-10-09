#!/bin/bash

# Copyright (c) 2008-2019 the MRtrix3 contributors.
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

set -ev

# Script expects environment variables ${py} and ${test} to be set

case  $test in
  "sphinx")
    $py -m sphinx -n -N -W -w sphinx.log docs/ tmp/
    ;;
  "syntax")
    ./check_syntax
    ;;
  "pylint")
    # Normally rely on build script to create this file
    echo "__version__ = 'pylint testing' #pylint: disable=unused-variable" > ./lib/mrtrix3/_version.py
    PYTHON=$py ./run_pylint
    ;;
  "build_docs")
    $py ./configure -nooptim && $py ./build -nowarnings -persistent -nopaginate && ./docs/generate_user_docs.sh && git diff --exit-code docs/ > gitdiff.log
    ;;
  "build")
    $py ./configure -nooptim && $py ./build -nowarnings -persistent -nopaginate
    ;;
  "run")
    $py ./configure -assert && $py ./build -nowarnings -persistent -nopaginate && ./run_tests
    ;;
  *)
    echo "Envvar \"test\" not defined";
    exit 1
    ;;
esac
