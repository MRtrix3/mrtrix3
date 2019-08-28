#!/bin/bash

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
