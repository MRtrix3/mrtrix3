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

const char* operations[] = {
  "sum",
  "count",
  NULL
};


void usage ()
{

  DESCRIPTION
  + "convert a fixel-based sparse-data image into a scalar image. Output either the sum of fixel values within a voxel, or the fixel count";

  ARGUMENTS
  + Argument ("fixel_in",  "the input sparse fixel image.").type_image_in ()
  + Argument ("operation", "the operation to apply, one of: " + join(operations, ", ") + ".").type_choice (operations)
  + Argument ("image_out", "the output scalar image.").type_image_out ();

}



void run ()
{
  Image::Header H_in (argument[0]);
  Image::BufferSparse<FixelMetric> fixel_data (H_in);
  Image::BufferSparse<FixelMetric>::voxel_type voxel (fixel_data);

  const size_t num_inputs = argument.size() - 2;
  const int op = argument[num_inputs];

  Image::Header H_out (H_in);
  if (op)
    H_out.datatype() = DataType::UInt8;
  else
    H_out.datatype() = DataType::Float32;

  Image::Buffer<float> out_data (argument[2], H_out);
  Image::Buffer<float>::voxel_type out (out_data);


  Image::LoopInOrder loop (voxel, "converting sparse fixel data to scalar image... ");
  for (loop.start (voxel, out); loop.ok(); loop.next (voxel, out)) {
    if (op) {
      out.value() = voxel.value().size();
    } else {
      float sum = 0;
      for (size_t fixel = 0; fixel != voxel.value().size(); ++fixel)
        sum += voxel.value()[fixel].value;
      out.value() = sum;
    }
  }
}

