/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt 2015

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
#include "point.h"
#include "progressbar.h"
#include "algo/loop.h"
#include "image.h"
#include "sparse/fixel_metric.h"
#include "sparse/keys.h"
#include "sparse/image.h"


using namespace MR;
using namespace App;

using Image::Sparse::FixelMetric;


void usage ()
{

  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  DESCRIPTION
  + "map the scalar value in each voxel to all fixels within that voxel"
    "This could be used to enable connectivity-based smoothing and enhancement to be performed on voxel-wise measures";

  ARGUMENTS
  + Argument ("image_in",  "the input image.").type_image_in ()
  + Argument ("fixel_in",  "the input fixel image.").type_image_in ()
  + Argument ("fixel_out", "the output fixel image.").type_image_out ();
}




void run ()
{
  Image::Buffer<float> scalar_buffer (argument[0]);
  auto scalar_vox = scalar_buffer.voxel();

  Image::Header fixel_header (argument[1]);
  Image::BufferSparse<FixelMetric> fixel_data (fixel_header);
  auto fixel_vox = fixel_data.voxel();

  Image::check_dimensions (scalar_buffer, fixel_header, 0, 3);

  Image::BufferSparse<FixelMetric> output_data (argument[2], fixel_header);
  auto output_vox = output_data.voxel();

  Image::LoopInOrder loop (scalar_vox, "mapping voxel scalar values to fixels ...", 0, 3);
  for (auto i = loop (scalar_vox, fixel_vox, output_vox); i; ++i) {
    output_vox.value().set_size (fixel_vox.value().size());
    for (size_t fixel = 0; fixel != fixel_vox.value().size(); ++fixel) {
      output_vox.value()[fixel] = fixel_vox.value()[fixel];
      output_vox.value()[fixel].value = scalar_vox.value();
    }
  }
}
