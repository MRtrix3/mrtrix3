#!/bin/bash

echo $#
echo $@

if [[ $# -ne 2 ]]; then
  echo usage: run.sh tag user
else

  set -ex
  sed -i'.orig' -e "s|TAGNAME|$1|g" -e "s|GIT_USER|$2|g" PKGBUILD
  makepkg

fi
