/*
 Copyright 2012 Brain Research Institute, Melbourne, Australia

 Written by David Raffelt, 23/02/2012.

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
#include "image/buffer.h"
#include "image/buffer_preload.h"
#include "image/voxel.h"
#include "image/filter/reslice.h"
#include "image/interp/cubic.h"
#include "image/interp/linear.h"
#include "image/interp/nearest.h"
#include "registration/linear_registration.h"
#include "registration/metric/mean_squared_metric.h"
#include "registration/transform/affine.h"
#include "math/matrix.h"

MRTRIX_APPLICATION

using namespace MR;
using namespace App;

const char* transformations[] = { "affine", "rigid", NULL };

void usage ()
{
  AUTHOR = "David Raffelt (d.raffelt@brain.org.au)";

  DESCRIPTION
      + "register two images together using an affine or rigid transformation model.";

  ARGUMENTS
      + Argument ("moving", "moving image").type_image_in ()
      + Argument ("target", "the target (fixed or template) image").type_image_in ()
      + Argument ("transform", "the output text file containing the transformation as a 4x4 matrix").type_file ()
      + Argument ("output", "the transformed moving image").type_image_out ();

  OPTIONS
      + Option ("nlevels", "the number of down-sampling levels. For example if 3 is specified, "
          "then registration will first run using images down-sampled by a factor "
          "of 4, then 2, then the full resolution. (Default: 1, no down-sampling)")
      + Argument ("num").type_integer ()

  + Option ("niter", "the maximum number of iterations. This can be specified either "
      "as a single number for all down-sampling levels, or a single"
      "value for each level.")
      + Argument ("num").type_sequence_int ()

  + Option ("transformation", "the transformation type. Valid choices are: affine and rigid."
      "(Default: affine)")
      + Argument ("type").type_choice (transformations)

  + Option ("tmask", "a mask to define the target image region to use for optimisation.")
      + Argument ("filename").type_image_in ()

  + Option ("mmask", "a mask to define the moving image region to use for optimisation.")
      + Argument ("filename").type_image_in ();
}

void run ()
{

  Image::BufferPreload<float> moving_data (argument[0]);
  Image::BufferPreload<float>::voxel_type moving_voxel (moving_data);
  Image::Interp::Linear<Image::BufferPreload<float>::voxel_type > moving_interp (moving_voxel);

  Image::BufferPreload<float> target_data (argument[1]);
  Image::BufferPreload<float>::voxel_type target_voxel (target_data);

  std::vector<int> niter (1);
  niter[0] = 300;
  Options opt = get_options ("niter");
  if (opt.size ()) {
    niter = parse_ints (opt[0][0]);
    for (size_t i = 0; i < niter.size (); ++i)
      if (niter[i] < 0)
        throw Exception ("the max number of iterations must be positive");
  }

  Registration::LinearRegistration registration (niter);
  Registration::Metric::MeanSquared metric;
  Registration::Transform::Affine<float> affine;


  Ptr<Image::BufferPreload<bool>::voxel_type> tmask_ptr;
  opt = get_options ("tmask");
  if (opt.size ()) {
    Image::BufferPreload<bool> tmask_data (opt[0][0]);
    Image::BufferPreload<bool>::voxel_type tmask_voxel (tmask_data);
    tmask_ptr = &tmask_voxel;
  }

  Ptr<Image::Interp::Nearest<Image::BufferPreload<bool>::voxel_type > > mmask_ptr;
  opt = get_options ("mmask");
  if (opt.size ()) {
    Image::BufferPreload<bool> mmask_data (opt[0][0]);
    Image::BufferPreload<bool>::voxel_type mmask_voxel (mmask_data);
    Image::Interp::Nearest<Image::BufferPreload<bool>::voxel_type> mmask_interp (mmask_voxel);
    mmask_ptr = &mmask_interp;
  }

  if (mmask_ptr && tmask_ptr) {
    registration.run_masked (metric, affine.get_parameter_vector(), moving_interp, target_voxel, &mmask_ptr, &tmask_ptr);
  }
  else if (tmask_ptr) {
    registration.run_target_mask (metric, affine.get_parameter_vector(), moving_interp, target_voxel, &tmask_ptr);
  }
  else if (mmask_ptr) {
    registration.run_moving_mask (metric, affine.get_parameter_vector(), moving_interp, target_voxel, &mmask_ptr);
  }
  else {
    registration.run (metric, affine.get_parameter_vector(), moving_interp, target_voxel);
  }

  affine.get_transform().save (argument[2]);

  Image::Header output_header (moving_data);
  output_header.info () = target_data.info ();
  Image::Buffer<float> output_data (argument[3], output_header);
  Image::Buffer<float>::voxel_type output_voxel (output_data);

  Image::Filter::reslice<Image::Interp::Cubic> (moving_voxel, output_voxel, affine.get_transform());

}
