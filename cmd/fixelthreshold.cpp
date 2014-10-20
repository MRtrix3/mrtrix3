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
  + "Threshold the values in a fixel image";

  ARGUMENTS
  + Argument ("fixel_in", "the input fixel image.").type_image_in ()
  + Argument ("threshold", "the input threshold").type_float()
  + Argument ("fixel",   "the output fixel image").type_image_out ();

  OPTIONS
  + Option ("crop", "remove fixels that fall below threshold (instead of assigning their value to zero or one)");

}



void run ()
{
  Image::Header input_header (argument[0]);
  Image::BufferSparse<FixelMetric> input_data (input_header);
  Image::BufferSparse<FixelMetric>::voxel_type input_vox (input_data);

  float threshold = argument[1];

  Image::BufferSparse<FixelMetric> output (argument[2], input_header);
  Image::BufferSparse<FixelMetric>::voxel_type output_vox (output);

  Options opt = get_options("crop");

  Image::LoopInOrder loop (input_vox, "thresholding fixel image...");
  for (loop.start (input_vox, output_vox); loop.ok(); loop.next (input_vox, output_vox)) {
    if (opt.size()) {
        size_t fixel_count = 0;
        for (size_t f = 0; f != input_vox.value().size(); ++f) {
          if (input_vox.value()[f].value > threshold)
            fixel_count++;
        }
        output_vox.value().set_size (fixel_count);
        fixel_count = 0;
        for (size_t f = 0; f != input_vox.value().size(); ++f) {
          if (input_vox.value()[f].value > threshold)
            output_vox.value()[fixel_count++] = input_vox.value()[f];
        }
    } else {
      output_vox.value().set_size (input_vox.value().size());
      for (size_t f = 0; f != input_vox.value().size(); ++f) {
        output_vox.value()[f] = input_vox.value()[f];
        if (input_vox.value()[f].value > threshold)
          output_vox.value()[f].value = 1.0;
        else
          output_vox.value()[f].value = 0.0;
      }
    }
  }

}

