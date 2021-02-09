#!/bin/bash

# Copyright (c) 2008-2021 the MRtrix3 contributors.
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

for s in 16 32 48 64 128; do
  for t in mrtrix mrtrix-gz nifti nifti-gz mgh mgz analyze; do #TODO add mrtrix-tracks
    xdg-icon-resource install --context apps --size $s icons/desktop/${s}x${s}/mrtrix.png application-x-${t}
  done
done

xdg-mime install mrtrix-mime.xml
xdg-desktop-menu install mrtrix-mrview.desktop
xdg-desktop-icon install mrtrix-mrview.desktop

