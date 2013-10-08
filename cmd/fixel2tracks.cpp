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

#include "app.h"
#include "progressbar.h"

#include "image/buffer.h"
#include "image/buffer_sparse.h"
#include "image/loop.h"
#include "image/voxel.h"

#include "image/sparse/fixel_metric.h"
#include "image/sparse/voxel.h"

#include "dwi/tractography/file.h"
#include "dwi/tractography/scalar_file.h"



MRTRIX_APPLICATION

using namespace MR;
using namespace App;



using Image::Sparse::FixelMetric;



void usage ()
{

  DESCRIPTION
  + "Generate small track segments (and corresponding track scalars) to visualise fixel directions and values";

  ARGUMENTS
  + Argument ("fixel_in", "the input sparse fixel image.").type_image_in ()

  + Argument ("tracks", "the output tract file ").type_file ()

  + Argument ("tsf", "the output track scalar file").type_image_out ();

}


#define ANGULAR_THRESHOLD 45



void run ()
{
  Image::Header input_header (argument[0]);
  Image::BufferSparse<FixelMetric> input_data (input_header);
  Image::BufferSparse<FixelMetric>::voxel_type input_fixel (input_data);

  float step = (input_fixel.vox(0) + input_fixel.vox(1) + input_fixel.vox(2)) / 6.0;

  DWI::Tractography::Properties tck_properties;
  DWI::Tractography::Writer<float> tck_writer (argument[1], tck_properties);

  DWI::Tractography::Properties tsf_properties;
  tsf_properties.timestamp = tck_properties.timestamp;
  DWI::Tractography::ScalarWriter<float> tsf_writer (argument[2], tsf_properties);

  Image::Transform transform (input_fixel);
  Point<float> voxel_pos;

  Image::LoopInOrder loop (input_fixel, "generating fixel-wise track segments");
  for (loop.start (input_fixel); loop.ok(); loop.next (input_fixel)) {
    for (size_t f = 0; f != input_fixel.value().size(); ++f) {
      std::vector<Point<float> > tck;
      transform.voxel2scanner (input_fixel, voxel_pos);
      tck.push_back (voxel_pos + (input_fixel.value()[f].dir * step));
      tck.push_back (voxel_pos + (input_fixel.value()[f].dir * -step));
      tck_writer.append (tck);
      std::vector<float> scalars;
      scalars.push_back(input_fixel.value()[f].value);
      scalars.push_back(input_fixel.value()[f].value);
      tsf_writer.append(scalars);
    }
  }

}

