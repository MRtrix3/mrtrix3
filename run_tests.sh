#!/bin/bash

LOGFILE=testing.log
echo logging to \""$LOGFILE"\" 

echo "ensuring build is up to date... "
./build > testing.log 2>&1

for n in testing/*; do
  if [ -d $n ]; then continue; fi
  script=$(basename $n)

  cat >> testing.log <<EOD
###########################################
  running ${script}...
###########################################
EOD

  echo -n "running ${script}... "
  ( 
    export PATH="$(pwd)/bin:$PATH"; 
    cd testing/data/ && exec ../$script
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
  else 
    echo OK
    cat >> testing.log <<EOD
##########################################
  completed OK
##########################################
EOD

  fi
done


