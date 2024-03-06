#!/bin/bash

# Copyright (c) 2008-2024 the MRtrix3 contributors.
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
  xdg-icon-resource install --context apps --size $s share/icons/hicolor/${s}x${s}/apps/application-x-mrtrix.png application-x-mrtrix
  xdg-icon-resource install --context mimetypes --size $s share/icons/hicolor/${s}x${s}/apps/application-x-mrtrix.png x-mrtrix
done

xdg-mime install share/mime/mrtrix-mime.xml
sed s^Exec=mrview^Exec=$(pwd)/bin/mrview^ < 'share/applications/mrview.desktop' > mrtrix-mrview.desktop
xdg-desktop-menu install mrtrix-mrview.desktop

