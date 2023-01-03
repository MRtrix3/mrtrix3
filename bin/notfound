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

if [ $# -eq 0 ]; then
  cat << 'HELP_PAGE'
USAGE:
  $ notfound base_directory search_string

  This is a simple script designed to help identify subjects that do not yet have a specific file generated. For example when adding new patients to a study. It is designed to be used when each patient has a folder containing their images.

  For example:
    $ notfound study_folder fod.mif
  will identify all subject folders (e.g. study_folder/subject001, study_folder/subject002, ...) that do NOT contain a file fod.mif

  Note that this can be used in combination with the foreach script. For example:
    $ foreach $(notfound study_folder fod.mif) : dwi2fod IN/dwi.mif IN/response.txt IN/fod.mif
HELP_PAGE

exit 1

fi

find ${1} -mindepth 1 -maxdepth 1 \( -type l -o -type d \) '!' -exec test -e "{}/${2}" ';' -print

