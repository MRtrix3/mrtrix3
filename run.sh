#!/bin/bash
if [[ $# -ne 2 ]]; then
  echo usage: run.sh tag user
else

  set -e
  sed -i "s|TAGNAME|$1|g" PKGBUILD
  sed -i "s|GIT_USER|$2|g" PKGBUILD
  makepkg

fi
