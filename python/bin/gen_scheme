#!/bin/bash

# Copyright (c) 2008-2023 the MRtrix3 contributors.
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

set -e

if [ "$#" -eq 0 ]; then
    echo "
    gen_scheme: part of the MRtrix package

SYNOPSIS

    gen_scheme numPE [ bvalue ndir ]...

       numPE        the number of phase-encoding directions to be included in
                    the scheme (most scanners will only support a single PE
                    direction per sequence, so this will typically be 1).

       bvalue       the b-value of the shell

       ndir         the number of directions to include in the shell


DESCRIPTION

    This script generates a diffusion gradient table according to the
    parameters specified. For most users, something like the following would be
    appropriate:

        gen_scheme 1 0 5 750 20 3000 60

    which will geneate a multi-shell diffusion gradient table with a single
    phase-encode direction, consisting of 5 b=0, 20 b=750, and 60 b=3000
    volumes.

    The gradient table is generated using the following procedure:

    - The directions for each shell are optimally distributed using a bipolar
      electrostatic repulsion model (using the command 'dirgen').

    - These are then split into numPE sets (if numPE != 1) using a brute-force
      random search for the most optimally-distributed subsets (using the command
      'dirsplit').

    - Each of the resulting sets is then rearranged by inversion of individual
      directions through the origin (i.e. direction vector x => -x) using a
      brute-force random search to find the most optimal combination in terms
      of unipolar repulsion: this ensures near-uniform distribution over the
      sphere to avoid biases in terms of eddy-current distortions, as
      recommended for FSL's EDDY command (this step uses the 'dirflip' command).

    - Finally, all the individual subsets are merged (using the 'dirmerge'
      command) into a single gradient table, in such a way as to maintain
      near-uniformity upon truncation (in as much as is possible), in both
      b-value and directional domains. In other words, the approach aims to
      ensure that if the acquisition is cut short, the set of volumes acquired
      nonetheless contains the same relative proportions of b-values as
      specified, with directions that are near-uniformly distributed.

    The primary output of this command is a file called 'dw_scheme.txt',
    consisting of a 5-column table, with one line per volume. Each column
    consists of [ x y z b PE ], where [ x y z ] is the unit direction vector, b
    is the b-value in unit of s/mm², and PE is a integer ID from 1 to numPE.

    The command also retains all of the subsets generated along the way, which
    you can safely delete once the command has completed. Since this can
    consist of quite a few files, it is recommended to run this command within
    its own temporary folder.

    See also the 'dirstat' command to obtain simple metrics of quality for the
    set produced.
"
    exit 1
else

  nPE=$1
  if [ $nPE -ne 1 ] && [ $nPE -ne 2 ] && [ $nPE -ne 4 ]; then
    echo "ERROR: numPE should be one of 1, 2, 4"
    exit 1
  fi

  shift
  # store args for re-use:
  ARGS=( "$@" )

  # print parsed info for sanity-checking:
  echo "generating scheme with $nPE phase-encode directions, with:"
  while [ ! -z "$1" ]; do
    echo "    b = $1: $2 directions"
    shift 2
  done

  perm="" #"-perm 1000"

  # reset args:
  set -- "${ARGS[@]}"
  merge=""

  while [ ! -z "$1" ]; do
    echo "====================================="
    echo "generating directions for b = $1..."
    echo "====================================="

    merge=$merge" "$1

    dirgen $2 dirs-b$1-$2.txt -force
    if [ $nPE -gt 1 ]; then
      dirsplit dirs-b$1-$2.txt dirs-b$1-$2-{1..2}.txt -force $perm
      if [ $nPE -gt 2 ]; then
        dirsplit dirs-b$1-$2-1.txt dirs-b$1-$2-1{1..2}.txt -force $perm
        dirsplit dirs-b$1-$2-2.txt dirs-b$1-$2-2{1..2}.txt -force $perm
        # TODO: the rest...
        for n in dirs-b$1-$2-{1,2}{1,2}.txt; do
          dirflip $n ${n%.txt}-flip.txt -force $perm
          merge=$merge" "${n%.txt}-flip.txt
        done
      else
        for n in dirs-b$1-$2-{1,2}.txt; do
          dirflip $n ${n%.txt}-flip.txt -force $perm
          merge=$merge" "${n%.txt}-flip.txt
        done
      fi
    else
      dirflip dirs-b$1-$2.txt dirs-b$1-$2-flip.txt -force $perm
      merge=$merge" "dirs-b$1-$2-flip.txt
    fi

    shift 2
  done

  echo $merge
  dirmerge $nPE $merge dw_scheme.txt -force
fi

