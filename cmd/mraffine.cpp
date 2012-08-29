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
#include "image/adapter/reslice.h"
#include "registration/linear_registration.h"
#include "registration/metric/mean_squared_metric.h"
#include "registration/transform/affine.h"
#include "registration/transform/rigid.h"
#include "math/vector.h"

MRTRIX_APPLICATION

using namespace MR;
using namespace App;
using namespace Registration;

const char* transformation_choices[] = { "rigid", "affine", "both", NULL };

const char* initialisation_choices[] = { "mass", "centre", "none",NULL };

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
  + Option ("scale", "use a multi-resolution scheme by defining a scale factor for each level "
                     "using comma separated values. For example to -scale 0.25,0.5,1. (Default: 0.5,1)")
  + Argument ("factor").type_sequence_float ()

  + Option ("transform", "the transformation type. Valid choices are: rigid, affine or both (initialise affine "
                         "using rigid result). (Default: affine)")
  + Argument ("type").type_choice (transformation_choices)

  + Option ("niter", "the maximum number of iterations. This can be specified either as a single number "
                     "for all multi-resolution levels, or a single value for each level. (Default: 1000)")
  + Argument ("num").type_sequence_int ()

    + Option ("tmask", "a mask to define the target image region to use for optimisation.")
  + Argument ("filename").type_image_in ()

  + Option ("mmask", "a mask to define the moving image region to use for optimisation.")
  + Argument ("filename").type_image_in ()

  + Option ("init", "initialise the centre of rotation and initial translation. Valid choices are: mass "
                    "(which uses the image center of mass), centre (geometric image centre) or none. "
                    "The default is mass (which may not be suited for multi-modality registration).")
  +Argument ("type").type_choice (initialisation_choices);
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

  std::vector<int> niter (1, 1000);
  Options opt = get_options ("niter");
  if (opt.size ()) {
    niter = parse_ints (opt[0][0]);
    for (size_t i = 0; i < niter.size (); ++i)
      if (niter[i] < 0)
        throw Exception ("the max number of iterations must be positive");
  }

  std::vector<float> scale_factors (2);
  scale_factors[0] = 0.5;
  scale_factors[1] = 1;
  opt = get_options ("scale");
  if (opt.size ()) {
    scale_factors = parse_floats (opt[0][0]);
    for (size_t i = 0; i < scale_factors.size(); ++i)
      if (scale_factors[i] < 0)
        throw Exception ("the multi-resolution scale factor must be positive");
  }

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

  LinearRegistration registration;
  registration.set_max_iter (niter);
  registration.set_scale_factor (scale_factors);

  int init = 0;
  opt = get_options ("init");
  if (opt.size())
    init = opt[0][0];
  switch (init) {
    case 0:
      registration.set_init_type (Transform::Init::mass);
      break;
    case 1:
      registration.set_init_type (Transform::Init::centre);
      break;
    case 2:
      registration.set_init_type (Transform::Init::none);
      break;
    default:
      break;
  }

  Metric::MeanSquared metric;

  int transform_type = 1;
  opt = get_options ("transform");
  if (opt.size())
    transform_type = opt[0][0];

  Math::Matrix <double> final_transform;
  if (transform_type == 0) {
    Transform::Rigid<double> rigid;
    CONSOLE ("running rigid body registration");
    registration.run_masked (metric, rigid, moving_voxel, target_voxel, mmask_voxel, tmask_voxel);
    rigid.get_transform (final_transform);
    final_transform.save (argument[2]);
    Image::Filter::reslice<Image::Interp::Cubic> (moving_voxel, output_voxel, final_transform, Image::Adapter::AutoOverSample, 0.0);
  } else if (transform_type == 1) {
    Transform::Affine<double> affine;
    CONSOLE ("running affine registration");
    registration.run_masked (metric, affine, moving_voxel, target_voxel, mmask_voxel, tmask_voxel);
    affine.get_transform (final_transform);
    final_transform.save (argument[2]);
    Image::Filter::reslice<Image::Interp::Cubic> (moving_voxel, output_voxel, final_transform, Image::Adapter::AutoOverSample, 0.0);
  } else {
    Transform::Rigid<double> rigid;
    CONSOLE ("running rigid body registration");
    registration.run_masked (metric, rigid, moving_voxel, target_voxel, mmask_voxel, tmask_voxel);
    rigid.get_transform (final_transform);
    Math::Vector<double> centre;
    rigid.get_centre (centre);
    Transform::Affine<double> affine;
    affine.set_centre (centre);
    affine.set_transform (final_transform);
    registration.set_init_type (Transform::Init::none);
    CONSOLE ("running affine registration");
    registration.run_masked (metric, affine, moving_voxel, target_voxel, mmask_voxel, tmask_voxel);
    affine.get_transform (final_transform);
    final_transform.save (argument[2]);
    Image::Filter::reslice<Image::Interp::Cubic> (moving_voxel, output_voxel, final_transform, Image::Adapter::AutoOverSample, 0.0);
  }

}
