#!/bin/bash

set -ev

# Script expects environment variables ${py} and ${test} to be set

if [ ${test} = "sphinx" ]; then
    $py -m sphinx -n -N -W -w sphinx.log -c docs/ docs/ tmp/;
elif [ ${test} = "memalign" ]; then
    ./check_memalign;
elif [ ${test} = "pylint" ]; then
    # Normally rely on build script to create this file
    echo "__version__ = pylint_testing" > ./lib/mrtrix3/_version.py
    PYTHON=$py ./run_pylint;
else
    $py ./configure -assert && $py ./build -nowarnings && ./run_tests;
    ./docs/generate_user_docs.sh && git diff --exit-code docs/ > gitdiff.log;
fi

exit 0
