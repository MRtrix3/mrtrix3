#!/bin/bash

if [[ $# -ne 2 ]]; then
  echo usage: run.sh tag user
  exit 1
else

  set -e
  sed -i'.orig' -e "s|TAGNAME|$1|g" -e "s|GIT_USER|$2|g" PKGBUILD
  makepkg -s

fi
