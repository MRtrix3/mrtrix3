/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 2013.

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

#include "command.h"
#include "progressbar.h"

#include "image/buffer.h"
#include "image/buffer_sparse.h"
#include "image/loop.h"
#include "image/voxel.h"

#include "image/sparse/fixel_metric.h"
#include "image/sparse/voxel.h"

#include "math/SH.h"




using namespace MR;
using namespace App;



using Image::Sparse::FixelMetric;



void usage ()
{

  DESCRIPTION
  + "convert a fixel-based sparse-data image into a scalar image showing the number of fibre populations in each voxel";

  ARGUMENTS
  + Argument ("fixel_in",  "the input sparse fixel image.").type_image_in ()
  + Argument ("image_out", "the output scalar image.").type_image_out ();

}




void run ()
{

  Image::Header H_in (argument[0]);
  Image::BufferSparse<FixelMetric> fixel_data (H_in);
  Image::BufferSparse<FixelMetric>::voxel_type fixel (fixel_data);

  Image::Header H_out (H_in);
  H_out.datatype() = DataType::UInt32;

  Image::Buffer<float> out_data (argument[1], H_out);
  Image::Buffer<float>::voxel_type out (out_data);

  Image::LoopInOrder loop (fixel, "converting sparse fixel data to fixel count image... ");
  for (loop.start (fixel, out); loop.ok(); loop.next (fixel, out))
    out.value() = fixel.value().size();

}

