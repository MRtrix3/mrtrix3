/*
    Copyright 2014 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt, 2014

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

using namespace MR;
using namespace App;

using Image::Sparse::FixelMetric;

void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  DESCRIPTION
  + "Divide two fixel images";

  ARGUMENTS
  + Argument ("input1", "the input fixel image.").type_image_in ()
  + Argument ("input2", "the input fixel image.").type_image_in ()
  + Argument ("output", "the input fixel image.").type_image_out ();
}


void run ()
{
  Image::Header input_header1 (argument[0]);
  Image::BufferSparse<FixelMetric> input_data1 (input_header1);
  Image::BufferSparse<FixelMetric>::voxel_type input_vox1 (input_data1);

  Image::Header input_header2 (argument[1]);
  Image::BufferSparse<FixelMetric> input_data2 (input_header2);
  Image::BufferSparse<FixelMetric>::voxel_type input_vox2 (input_data2);

  Image::check_dimensions(input_header1, input_header2);

  Image::BufferSparse<FixelMetric> output_data (argument[2], input_header1);
  Image::BufferSparse<FixelMetric>::voxel_type output_vox (output_data);

  Image::LoopInOrder loop (input_data1, "dividing fixel images...");
  for (loop.start (input_vox1, input_vox2, output_vox); loop.ok(); loop.next (input_vox1, input_vox2, output_vox)) {
    if (input_vox1.value().size() != input_vox2.value().size())
      throw Exception ("the fixel images do not have corresponding fixels in all voxels");
    output_vox.value().set_size (input_vox1.value().size());
    for (size_t fixel = 0; fixel != input_vox1.value().size(); ++fixel) {
      output_vox.value()[fixel] = input_vox1.value()[fixel];
      output_vox.value()[fixel].value = input_vox1.value()[fixel].value / input_vox2.value()[fixel].value;
    }
  }
}

