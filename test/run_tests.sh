#!/bin/bash

set -e
./build

for n in scripts/*; do
  echo '####################################'
  echo '###' running $n ...
  echo '####################################'
  echo ''
  PATH="$(pwd)/../bin:$(pwd)/bin:$PATH" $n

  echo ''
  echo '###' $n completed successfully
  echo ''
done


