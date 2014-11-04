#!/bin/bash

for s in 16 32 48 64 128; do
  for t in mrtrix mrtrix-gz nifti nifti-gz mgh mgz analyze; do #TODO add mrtrix-tracks
    xdg-icon-resource install --context apps --size $s icons/desktop/${s}x${s}/mrtrix.png application-x-${t}
  done
done

xdg-mime install mrtrix-mime.xml
xdg-desktop-menu install mrtrix-mrview.desktop
xdg-desktop-icon install mrtrix-mrview.desktop

