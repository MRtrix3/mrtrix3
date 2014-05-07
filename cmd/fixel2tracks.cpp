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

#include "dwi/tractography/file.h"
#include "dwi/tractography/scalar_file.h"




using namespace MR;
using namespace App;



using Image::Sparse::FixelMetric;



void usage ()
{

  DESCRIPTION
  + "Generate small track segments (and corresponding track scalars) to visualise fixel directions and values";

  ARGUMENTS
  + Argument ("fixel_in", "the input sparse fixel image.").type_image_in ()
  + Argument ("tracks",   "the output track file ")       .type_file ();


  OPTIONS
  + Option ("tsf", "output an accompanying track scalar file containing the fixel values")
    + Argument ("path").type_image_out ()

  + Option ("length", "vary the length of each track according to the fixel value")

  + Option ("scale", "scale the length of each track by a multiplicative factor")
    + Argument ("value").type_float (1e-6, 1.0, 1e+6);

}



void run ()
{
  Image::Header input_header (argument[0]);
  Image::BufferSparse<FixelMetric> input_data (input_header);
  Image::BufferSparse<FixelMetric>::voxel_type input_fixel (input_data);

  float half_length = 0.5 * Math::sqrt (Math::pow2 (input_fixel.vox(0)) + Math::pow2 (input_fixel.vox(1)) + Math::pow2(input_fixel.vox(2)));
  Options opt = get_options ("scale");
  if (opt.size())
    half_length *= float(opt[0][0]);

  DWI::Tractography::Properties properties;
  properties.comments.push_back ("Created using fixel2tracks");
  properties.comments.push_back ("Source fixel image: " + Path::basename (argument[0]));
  DWI::Tractography::Writer<float> tck_writer (argument[1], properties);

  Ptr< DWI::Tractography::ScalarWriter<float> > tsf_writer;
  opt = get_options ("tsf");
  if (opt.size())
    tsf_writer = new DWI::Tractography::ScalarWriter<float> (opt[0][0], properties);

  opt = get_options ("length");
  bool scale_length_by_value = opt.size();

  Image::Transform transform (input_fixel);
  Point<float> voxel_pos;

  Image::LoopInOrder loop (input_fixel, "generating fixel-wise track segments");
  for (loop.start (input_fixel); loop.ok(); loop.next (input_fixel)) {
    for (size_t f = 0; f != input_fixel.value().size(); ++f) {
      DWI::Tractography::Streamline<float> tck;
      transform.voxel2scanner (input_fixel, voxel_pos);
      const float step = half_length * (scale_length_by_value ? input_fixel.value()[f].value : 1.0);
      tck.push_back (voxel_pos + (input_fixel.value()[f].dir *  step));
      tck.push_back (voxel_pos + (input_fixel.value()[f].dir * -step));
      tck_writer (tck);
      if (tsf_writer) {
        std::vector<float> scalars;
        scalars.push_back (input_fixel.value()[f].value);
        scalars.push_back (input_fixel.value()[f].value);
        (*tsf_writer) (scalars);
      }
    }
  }

}

