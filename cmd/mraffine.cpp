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
#include "image/transform.h"
#include "registration/linear_registration.h"
#include "registration/metric/mean_squared_metric.h"
#include "registration/transform/affine.h"
#include "registration/transform/initialiser.h"
#include "math/vector.h"

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
  + Option ("niter", "the maximum number of iterations. This can be specified either "
      "as a single number for all multi-resolution levels, or a single value for each level. (Default: 300)")
  + Argument ("num").type_sequence_int ()

  + Option ("resolution", "setup the multi-resolution scheme by defining a scale factor for each level "
                          "using comma separated values. Default: 0.5,1")
  + Argument ("factor").type_sequence_float ()

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

  Image::BufferPreload<float> target_data (argument[1]);
  Image::BufferPreload<float>::voxel_type target_voxel (target_data);

  Image::Header output_header (target_data);
  Image::Buffer<float> output_data (argument[3], output_header);
  Image::Buffer<float>::voxel_type output_voxel (output_data);

  std::vector<int> niter (1, 300);
  Options opt = get_options ("niter");
  if (opt.size ()) {
    niter = parse_ints (opt[0][0]);
    for (size_t i = 0; i < niter.size (); ++i)
      if (niter[i] < 0)
        throw Exception ("the max number of iterations must be positive");
  }

  std::vector<float> resolution (2);
  resolution[0] = 0.5;
  resolution[1] = 1;
  opt = get_options ("resolution");
  if (opt.size ()) {
    resolution = parse_floats (opt[0][0]);
    for (size_t i = 0; i < resolution.size(); ++i)
      if (resolution[i] < 0)
        throw Exception ("the multi-resolution scale factor must be positive");
  }

  Registration::Metric::MeanSquared metric;
  Registration::Transform::Affine<double> affine;
  Registration::Transform::initialise_using_image_centres (moving_voxel, target_voxel, affine);

  Ptr<Image::BufferPreload<bool> > tmask_data;
  Ptr<Image::BufferPreload<bool>::voxel_type> tmask_voxel;
  opt = get_options ("tmask");
  if (opt.size ()) {
    tmask_data = new Image::BufferPreload<bool> (opt[0][0]);
    tmask_voxel = new Image::BufferPreload<bool>::voxel_type (*tmask_data);
  }

  Ptr<Image::BufferPreload<bool> > mmask_data;
  Ptr<Image::BufferPreload<bool>::voxel_type> mmask_voxel;
  opt = get_options ("mmask");
  if (opt.size ()) {
    mmask_data = new Image::BufferPreload<bool> (opt[0][0]);
    mmask_voxel = new Image::BufferPreload<bool>::voxel_type (*mmask_data);
  }

  Registration::LinearRegistration registration;
  registration.set_max_iter (niter);
  registration.set_resolution (resolution);

  registration.run_masked (metric, affine, moving_voxel, target_voxel, mmask_voxel, tmask_voxel);

  affine.get_transform().save (argument[2]);
  Math::Matrix<double> inv;
  Math::LU::inv (inv, affine.get_transform());

  Image::Filter::reslice<Image::Interp::Cubic> (moving_voxel, output_voxel, inv);
}
