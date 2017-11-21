#!/bin/bash

set -ev

# Script expects environment variables ${py} and ${test} to be set

case  $test in
  "sphinx")
    $py -m sphinx -n -N -W -w sphinx.log -c docs/ docs/ tmp/
    ;;
  "memalign")
    ./check_memalign
    ;;
  "pylint")
    # Normally rely on build script to create this file
    echo "__version__ = pylint_testing" > ./lib/mrtrix3/_version.py
    PYTHON=$py ./run_pylint
    ;;
  "build")
    $py ./configure -nooptim && $py ./build -nowarnings
    ;;
  "run")
    $py ./configure -assert && $py ./build -nowarnings && ./run_tests && ./docs/generate_user_docs.sh && git diff --exit-code docs/ > gitdiff.log
    ;;
  *)
    echo "Envvar \"test\" not defined";
    exit 1
    ;;
esac
