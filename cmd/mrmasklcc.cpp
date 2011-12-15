/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert Smith, 2011.

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

#include "dataset/buffer.h"
#include "dataset/copy.h"
#include "dataset/loop.h"
#include "image/header.h"
#include "image/voxel.h"
#include "filter/lcc.h"


using namespace MR;
using namespace App;

MRTRIX_APPLICATION

void usage ()
{

  DESCRIPTION
  + "Reads a generated mask image (typically from a thresholded average-DWI image), and outputs a mask containing only the largest structure in the mask (presumably the brain). "
  + "It also fills in any gaps within the brain mask caused by the thresholding process.";

  ARGUMENTS
  + Argument ("input",  "the input mask image") .type_image_in()
  + Argument ("output", "the output mask image").type_image_out();

  OPTIONS
  + Option ("fill", "In addition to extracting the largest connected-component of the mask, also fill in any gaps within this mask");

}



void run ()
{

  Image::Header H_in (argument[0]);
  Image::Voxel<bool> v_in (H_in);

  DataSet::Buffer<bool> largest_mask (H_in);

  {
    Filter::LargestConnectedComponent<bool, Image::Voxel<bool>, DataSet::Buffer<bool> > lcc (v_in, "getting largest connected-component...");
    lcc.execute (largest_mask);
  }

  if (get_options ("fill").size()) {

    DataSet::Loop loop;
    for (loop.start (largest_mask); loop.ok(); loop.next (largest_mask))
      largest_mask.value() = !largest_mask.value();

    DataSet::Buffer<bool> outside_mask (H_in);

    Filter::LargestConnectedComponent<bool, DataSet::Buffer<bool>, DataSet::Buffer<bool> > lcc_fill (largest_mask, "filling gaps in mask...");
    lcc_fill.execute (outside_mask);
    for (loop.start (outside_mask, largest_mask); loop.ok(); loop.next (outside_mask, largest_mask))
      largest_mask.value() = !outside_mask.value();

  }

  Image::Header H_out (H_in);
  H_out.create (argument[1]);
  Image::Voxel<bool> v_out (H_out);
  DataSet::copy (v_out, largest_mask);

}

