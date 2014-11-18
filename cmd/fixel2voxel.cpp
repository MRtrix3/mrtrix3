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
  "split",
  NULL
};


void usage ()
{

  DESCRIPTION
  + "convert a fixel-based sparse-data image into a scalar image. "
    "Output either the sum of fixel values within a voxel, the fixel count, "
    "or a set of 3D scalar images, one per fixel value.";

  ARGUMENTS
  + Argument ("fixel_in",  "the input sparse fixel image.").type_image_in ()
  + Argument ("operation", "the operation to apply, one of: " + join(operations, ", ") + ".").type_choice (operations)
  + Argument ("image_out", "the output scalar image.").type_image_out ();

}



void run ()
{
  Image::Header H_in (argument[0]);
  Image::BufferSparse<FixelMetric> fixel_data (H_in);
  auto voxel = fixel_data.voxel();

  const int op = argument[1];

  Image::Header H_out (H_in);
  if (op == 1)
    H_out.datatype() = DataType::UInt8;
  else {
    H_out.datatype() = DataType::Float32;
    if (op == 2) {
      H_out.set_ndim (4);
      uint32_t max_count = 0;
      Image::LoopInOrder loop (voxel, "determining largest fixel count... ");
      for (loop.start (voxel); loop.ok(); loop.next (voxel)) 
        max_count = std::max (max_count, voxel.value().size());
      if (max_count == 0) 
        throw Exception ("fixel image is empty");
      H_out.dim(3) = max_count;
    }
  }

  Image::Buffer<float> out_data (argument[2], H_out);

  auto out = out_data.voxel();

  Image::LoopInOrder loop (voxel, "converting sparse fixel data to scalar image... ");
  for (auto i = loop (voxel, out); i; ++i) {
    switch (op) {
      case 0: // sum
        {
          float sum = 0.0;
          for (size_t fixel = 0; fixel != voxel.value().size(); ++fixel)
            sum += voxel.value()[fixel].value;
          out.value() = sum;
        }
        break;
      case 1: // count
        out.value() = voxel.value().size();
        break;
      case 2: // split
        for (out[3] = 0; out[3] < out.dim(3); ++out[3]) {
          if (out[3] < voxel.value().size())
            out.value() = voxel.value()[out[3]].value;
          else
            out.value() = 0.0;
        }
    }
  }
}

