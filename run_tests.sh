#!/bin/bash

LOGFILE=testing.log
echo logging to \""$LOGFILE"\" 

echo -n "ensuring build is up to date... "
./build > testing.log 2>&1
if [ $? != 0 ]; then 
  echo ERROR!
  exit 1
else
  echo OK
fi

for n in testing/*; do
  if [ -d $n ]; then continue; fi
  script=$(basename $n)
  rm -f testing/data/tmp.*

  cat >> testing.log <<EOD
###########################################
  running ${script}...
###########################################
EOD

  echo -n "running ${script}... "
  ( 
    set -ex
    export PATH="$(pwd)/bin:$PATH"; 
    cd testing/data/
    source ../$script
  ) > .__tmp.log 2>&1
  error=$?


  cat .__tmp.log >> testing.log
  if [ $error != 0 ]; then 
    echo ERROR! 
    cat >> testing.log <<EOD
##########################################
  ERROR!
##########################################
EOD
    exit 1
  else 
    echo OK
    cat >> testing.log <<EOD
##########################################
  completed OK
##########################################
EOD

  fi
done


