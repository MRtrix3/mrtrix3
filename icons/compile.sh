#!/bin/bash

echo '<!DOCTYPE RCC><RCC version="1.0">' > mrview.qrc
echo '<qresource>' >> mrview.qrc
for name in *.svg *.png; do
  echo '<file>'$name'</file>' >> mrview.qrc
done
echo '</qresource>' >> mrview.qrc
echo '</RCC>' >> mrview.qrc

rcc mrview.qrc -o ../src/gui/mrview_icons.cpp
rcc disp_profile.qrc -o ../src/gui/disp_profile_icon.cpp

