/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt

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
  DESCRIPTION
  + "Count the number of fixels in an image";

  ARGUMENTS
  + Argument ("fixel_in", "the input fixel image.").type_image_in ();
}



void run ()
{
  Image::Header input_header (argument[0]);
  Image::BufferSparse<FixelMetric> input_data (input_header);
  Image::BufferSparse<FixelMetric>::voxel_type input_vox (input_data);

  size_t count = 0;

  {
    Image::LoopInOrder loop (input_vox, "counting fixels");
    for (loop.start (input_vox); loop.ok(); loop.next (input_vox)) {
      count += input_vox.value().size();
    }
  }

  CONSOLE(str(count));

}

