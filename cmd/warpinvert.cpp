/*
 Copyright 2012 Brain Research Institute, Melbourne, Australia

 Written by David Raffelt, 20/11/2012

 This file is part of MRtrix.

 MRtrix is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 MRtrix is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

 */

#include "app.h"
#include "image/buffer.h"
#include "registration/transform/warp_inverter.h"

MRTRIX_APPLICATION

using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "David Raffelt (d.raffelt@brain.org.au)";

  DESCRIPTION
    + "compute the inverse of a warp field";

  ARGUMENTS
    + Argument ("warp", "the input 4D warp image").type_image_in ()
    + Argument ("invwarp", "the output inverse warp image").type_image_in ();

}

typedef float value_type;


void run ()
{

  Image::Buffer<float> warp (argument[0]);
  Image::Buffer<float>::voxel_type warp_vox (warp);

  Image::Registration::WarpInverter inverter (warp_vox);

  Image::Header header (warp);
  header.info() = inverter.info();
  Image::Buffer<float> inv_warp (argument[1], header);
  Image::Buffer<float>::voxel_type inv_warp_vox (inv_warp);

  inverter (warp_vox, inv_warp_vox);
}
