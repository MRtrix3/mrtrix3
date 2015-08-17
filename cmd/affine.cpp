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

#include "command.h"
#include "image/buffer.h"
#include "image/buffer_scratch.h"
#include "image/buffer_preload.h"
#include "image/voxel.h"
#include "image/filter/reslice.h"
#include "image/interp/cubic.h"
#include "image/interp/nearest.h"
#include "image/transform.h"
#include "image/adapter/reslice.h"
#include "image/registration/linear.h"
#include "image/registration/syn.h"
#include "image/registration/metric/mean_squared.h"
#include "image/registration/metric/cross_correlation.h"
#include "image/registration/metric/mutual_information.h"
#include "image/registration/metric/mean_squared_4D.h"
#include "image/registration/transform/affine.h"
#include "image/registration/transform/rigid.h"
#include "dwi/directions/predefined.h"
#include "math/vector.h"
#include "math/matrix.h"
#include "math/SH.h"
#include "math/LU.h"

#include "image/registration_symmetric/linear.h"
#include "image/registration_symmetric/metric/mean_squared.h"
#include "image/registration_symmetric/metric/mean_squared_4D.h"
#include "image/registration_symmetric/transform/affine.h"
#include "image/registration_symmetric/transform/rigid.h"


using namespace MR;
using namespace App;

const char* transformation_choices[] = { "rigid", "affine", nullptr };


void usage ()
{
  AUTHOR = "David Raffelt (d.raffelt@brain.org.au), oh and Max";

  DESCRIPTION
      + "Register two images together using a rigid or affine transformation model."

      + "By default this application will perform an affine registration."

      + "By default the affine transformation will be saved in the warp image header (use mrinfo to view). To save the affine transform "
        "separately as a text file, use the -affine_out option.";


  ARGUMENTS
    + Argument ("moving", "moving image").type_image_in ()
    + Argument ("template", "template image").type_image_in ();


  OPTIONS
  + Option ("type", "the registration type. Valid choices are: "
                             "rigid, affine (Default: rigid)")
    + Argument ("choice").type_choice (transformation_choices)

  + Option ("transformed", "the transformed moving (FIXME: header is that of template) image after registration to the template")
    + Argument ("image").type_image_out ()

  + Option ("tmask", "a mask to define the template image region to use for optimisation.")
    + Argument ("filename").type_image_in ()

  + Option ("mmask", "a mask to define the moving image region to use for optimisation.")
    + Argument ("filename").type_image_in ()

  + Option ("gradient_descent_log_file", "logfile for gradient descent.")
    + Argument ("filename").type_file_out ()

  + Option ("symmetric", "use symmetric metric registration")

  + Image::Registration::rigid_options

  + Image::Registration::affine_options

  // + Image::Registration::syn_options

  + Image::Registration::initialisation_options

  + DataType::options();

  // + Image::Registration::fod_options;
}

/*
==============
notes:
==============

http://en.wikipedia.org/wiki/Versor


mrtransform:  Note the standard 'reverse' convention is used, where the transform maps
              points in the template image to the moving image.
*/

typedef float value_type;
typedef Image::Buffer<value_type> BufferIn;
typedef Image::Buffer<value_type> BufferOut;

void load_image (std::string filename, size_t num_vols, std::unique_ptr<Image::BufferScratch<value_type> >& image_buffer) {
  BufferIn buffer (filename);
  BufferIn::voxel_type vox (buffer);
  Image::Header header (filename);
  Image::Info info (header);
  if (num_vols > 1) {
    info.dim(3) = num_vols;
    info.stride(0) = 2;
    info.stride(1) = 3;
    info.stride(2) = 4;
    info.stride(3) = 1;
  }
  image_buffer.reset (new Image::BufferScratch<value_type> (info));
  Image::BufferScratch<value_type>::voxel_type image_vox (*image_buffer);
  if (num_vols > 1) {
    Image::LoopInOrder loop (vox, 0, 3);
    for (loop.start (vox, image_vox); loop.ok(); loop.next (vox, image_vox)) {
      for (size_t vol = 0; vol < num_vols; ++vol) {
        vox[3] = image_vox[3] = vol;
        image_vox.value() = vox.value();
      }
    }
  } else {
    Image::threaded_copy (vox, image_vox);
  }
}


void run ()
{
  Image::Header moving_header (argument[0]);
  moving_header.datatype() = DataType::from_command_line(DataType::Float32);
  const Image::Header template_header (argument[1]);

  Image::check_dimensions (moving_header, template_header); // TODO: replace this with common bounding box calculation --> mid-way space
  std::unique_ptr<Image::BufferScratch<value_type> > moving_buffer_ptr;
  std::unique_ptr<Image::BufferScratch<value_type> > template_buffer_ptr;


  Options opt = get_options ("noreorientation");
  bool do_reorientation = true;
  if (opt.size())
    do_reorientation = false;

  opt = get_options ("rigid_cc");
  bool use_crossCorrelation  = false;
  if (opt.size())
    use_crossCorrelation = true;


  if (template_header.ndim() > 4) {
    throw Exception ("image dimensions larger than 4 are not supported");
  }
  else if (template_header.ndim() == 4) {
    throw Exception ("SH registration not yet implemented");
  }
  else {
    do_reorientation = false;
    load_image (argument[0], 1, moving_buffer_ptr);
    load_image (argument[1], 1, template_buffer_ptr);
  }

  auto moving_voxel = moving_buffer_ptr->voxel();
  auto template_voxel = template_buffer_ptr->voxel();

  opt = get_options ("transformed");
  std::unique_ptr<Image::Buffer<value_type> > transformed_buffer_ptr;
  if (opt.size())
    transformed_buffer_ptr.reset (new Image::Buffer<value_type> (opt[0][0], moving_header)); // was template_header

  opt = get_options ("type");
  bool do_rigid      = false;
  bool do_affine     = false;
  bool do_affine_sym = false;
  // bool do_syn = false;
  int registration_type = 0;
  if (opt.size())
    registration_type = opt[0][0];
  switch (registration_type) {
    case 0:
      do_rigid = true;
      break;
    case 1:
      do_affine = true;
      break;
    default:
      break;
  }
  opt = get_options ("symmetric");
  if (opt.size())
    do_affine_sym = true;

  opt = get_options ("rigid_out");
  bool output_rigid = false;
  std::string rigid_filename;
  if (opt.size()) {
    if (!do_rigid)
      throw Exception ("rigid transformation output requested when no rigid registration is requested");
    output_rigid = true;
    rigid_filename = std::string (opt[0][0]);
  }

  opt = get_options ("affine_out");
  bool output_affine = false;
  std::string affine_filename;
  if (opt.size()) {
   if (!do_affine)
     throw Exception ("affine transformation output requested when no affine registration is requested");
   output_affine = true;
   affine_filename = std::string (opt[0][0]);
  }



  opt = get_options ("rigid_scale");
  std::vector<value_type> rigid_scale_factors;
  if (opt.size ()) {
    if (!do_rigid)
      throw Exception ("the rigid multi-resolution scale factors were input when no rigid registration is requested");
    rigid_scale_factors = parse_floats (opt[0][0]);
  }

  opt = get_options ("affine_scale");
  std::vector<value_type> affine_scale_factors;
  if (opt.size ()) {
    if (!do_affine)
      throw Exception ("the affine multi-resolution scale factors were input when no rigid registration is requested");
    affine_scale_factors = parse_floats (opt[0][0]);
  }


  opt = get_options ("tmask");
  std::unique_ptr<Image::BufferPreload<bool> > tmask_image;
  if (opt.size ())
    tmask_image.reset (new Image::BufferPreload<bool> (opt[0][0]));

  opt = get_options ("mmask");
  std::unique_ptr<Image::BufferPreload<bool> > mmask_image;
  if (opt.size ())
    mmask_image.reset (new Image::BufferPreload<bool> (opt[0][0]));

  opt = get_options ("rigid_niter");
  std::vector<int> rigid_niter;;
  if (opt.size ()) {
    rigid_niter = parse_ints (opt[0][0]);
    if (!do_rigid)
      throw Exception ("the number of rigid iterations have been input when no rigid registration is requested");
  }

  opt = get_options ("affine_niter");
  std::vector<int> affine_niter;
  if (opt.size ()) {
    affine_niter = parse_ints (opt[0][0]);
    if (!do_affine)
      throw Exception ("the number of affine iterations have been input when no affine registration is requested");
  }

  Image::Registration::Transform::Rigid<double> rigid;
  opt = get_options ("rigid_init");
  bool init_rigid_set = false;
  if (opt.size()) {
    throw Exception ("initialise with rigid not yet implemented");
    init_rigid_set = true;
    Math::Matrix<value_type> init_rigid;
    init_rigid.load (opt[0][0]);
    //TODO // set rigid
  }



  Image::RegistrationSymmetric::Transform::Affine<double> affineSym;
  Image::Registration::Transform::Affine<double> affine;


  opt = get_options ("affine_init");
  bool init_affine_set = false;
  if (opt.size()) {
    throw Exception ("initialise with affine not yet implemented");
    if (init_rigid_set)
      throw Exception ("you cannot initialise registrations with both a rigid and affine transformation");
    if (do_rigid)
      throw Exception ("you cannot initialise a rigid registration with an affine transformation");
    init_affine_set = true;
    Math::Matrix<value_type> init_affine;
    init_affine.load (opt[0][0]);
    //TODO // set affine
  }

  opt = get_options ("centre");
  Image::Registration::Transform::Init::InitType init_centre = Image::Registration::Transform::Init::mass;
  auto init_centreSym = Image::RegistrationSymmetric::Transform::Init::mass;
  if (opt.size()) {
    switch ((int)opt[0][0]){
      case 0:
        init_centre = Image::Registration::Transform::Init::mass;
        init_centreSym = Image::RegistrationSymmetric::Transform::Init::mass;
        break;
      case 1:
        init_centre = Image::Registration::Transform::Init::geometric;
        init_centreSym = Image::RegistrationSymmetric::Transform::Init::geometric;
        break;
      case 2:
        init_centre = Image::Registration::Transform::Init::none;
        init_centreSym = Image::RegistrationSymmetric::Transform::Init::none;
        break;
      default:
        break;
    }
  }

  opt = get_options ("gradient_descent_log_file");
  std::ofstream ofs;
  if (opt.size ())
    ofs.open(opt[0][0]); // std::cout

  Math::Matrix<value_type> directions_cartesian;
  if (do_reorientation) {
    Math::Matrix<value_type> directions_el_az;
    opt = get_options ("directions");
    if (opt.size())
      directions_el_az.load (opt[0][0]);
    else
      DWI::Directions::electrostatic_repulsion_60 (directions_el_az);
    Math::SH::S2C (directions_el_az, directions_cartesian);
  }

  if (do_rigid) {
    CONSOLE ("running rigid registration");
    Image::Registration::Linear rigid_registration;

    if (rigid_scale_factors.size())
      rigid_registration.set_scale_factor (rigid_scale_factors);
    if (rigid_niter.size()){
      rigid_registration.set_max_iter (rigid_niter);
    }
    rigid_registration.set_grad_tolerance(1e-5);
    // rigid_registration.set_step_tolerance(1e-10);
    // CONSOLE("rigid grad_tolerance: " + str(1e-5));

    if (init_rigid_set)
      rigid_registration.set_init_type (Image::Registration::Transform::Init::none);
    else
      rigid_registration.set_init_type (init_centre);

    if (template_voxel.ndim() == 4) {
      if (use_crossCorrelation){
        throw Exception ("CrossCorrelation4D not yet implemented");
        // Image::Registration::Metric::CrossCorrelation4D metric;
      }
      Image::Registration::Metric::MeanSquared4D metric;
      rigid_registration.run_masked (metric, rigid, moving_voxel, template_voxel, mmask_image, tmask_image);
    } else {
      // auto get_metric = [use_crossCorrelation] () { 
      //   if (use_crossCorrelation){
      //     CONSOLE ("metric: cross correlation");
      //     return Image::Registration::Metric::MeanSquared metric;
      //   } else {
      //     return Image::Registration::Metric::MeanSquared metric;
      //   }
      // };
      // auto metric = get_metric();
      // #include <type_traits> //for std::conditional


      // std::unique_ptr<Image::Registration::Metric::Base> metric(nullptr);
      // if (use_crossCorrelation){
      //   metric.reset(new ::Image::Registration::Metric::CrossCorrelation);
      // } else {
      //   metric.reset(new ::Image::Registration::Metric::MeanSquared);
      // }
      //  rigid_registration.run_masked (metric, rigid, moving_voxel, template_voxel, mmask_image, tmask_image); 
      // }
      if (use_crossCorrelation){
        CONSOLE ("  metric: cross correlation");
        Image::Registration::Metric::CrossCorrelation metric;
        // Image::Registration::Metric::MutualInformation metric;
        std::vector<size_t> extent (3,3);
        rigid_registration.set_extent(extent);
        rigid_registration.run_masked (metric, rigid, moving_voxel, template_voxel, mmask_image, tmask_image);
      } else {
        Image::Registration::Metric::MeanSquared metric;
        rigid_registration.run_masked (metric, rigid, moving_voxel, template_voxel, mmask_image, tmask_image);
      }
    }
    if (output_rigid)
      rigid.get_transform().save (rigid_filename);
  }

  if (do_affine && do_affine_sym) {
    CONSOLE ("running symmetric affine registration");
    Image::RegistrationSymmetric::Linear affine_registration;

    // affine_registration.set_grad_tolerance(0.1);
    affine_registration.set_step_tolerance(1e-16);
    affine_registration.set_grad_tolerance(0.0001);
    affine_registration.set_gradient_criterion_tolerance(0.0001);
    affine_registration.set_relative_cost_improvement_tolerance(1e-10);

    if (get_options ("gradient_descent_log_file").size())
      affine_registration.set_gradient_descent_log_stream(ofs.rdbuf()); 

    if (affine_scale_factors.size())
      affine_registration.set_scale_factor (affine_scale_factors);
    if (affine_niter.size())
      affine_registration.set_max_iter (affine_niter);
    if (do_rigid) {
      affineSym.set_centre (rigid.get_centre());
      affineSym.set_translation (rigid.get_translation());
      affineSym.set_matrix (rigid.get_matrix());
    }
    if (do_rigid || init_affine_set)
      affine_registration.set_init_type (Image::RegistrationSymmetric::Transform::Init::none);
    else
      affine_registration.set_init_type (init_centreSym);

    if (do_reorientation)
      affine_registration.set_directions (directions_cartesian);


    if (template_voxel.ndim() == 4) {
      Image::RegistrationSymmetric::Metric::MeanSquared4D metric;
      INFO("Image::RegistrationSymmetric::Metric::MeanSquared4D");
      affine_registration.run_masked (metric, affineSym, moving_voxel, template_voxel, mmask_image, tmask_image);
    } else {
      INFO("Image::RegistrationSymmetric::Metric::MeanSquared");
      Image::RegistrationSymmetric::Metric::MeanSquared metric;
      affine_registration.run_masked (metric, affineSym, moving_voxel, template_voxel, mmask_image, tmask_image);
    }

    if (get_options ("gradient_descent_log_file").size())
      ofs.close();

    if (output_affine)
      affineSym.get_transform().save (affine_filename);
  }

  if (do_affine && !do_affine_sym) {
    CONSOLE ("running affine registration");
    Image::Registration::Linear affine_registration;

    if (get_options ("gradient_descent_log_file").size())
      affine_registration.set_gradient_descent_log_stream(ofs.rdbuf());

    if (affine_scale_factors.size())
      affine_registration.set_scale_factor (affine_scale_factors);
    if (affine_niter.size())
      affine_registration.set_max_iter (affine_niter);
    if (do_rigid) {
      affine.set_centre (rigid.get_centre());
      affine.set_translation (rigid.get_translation());
      affine.set_matrix (rigid.get_matrix());
    }
    if (do_rigid || init_affine_set)
      affine_registration.set_init_type (Image::Registration::Transform::Init::none);
    else
      affine_registration.set_init_type (init_centre);

    if (do_reorientation)
      affine_registration.set_directions (directions_cartesian);


    if (template_voxel.ndim() == 4) {
      Image::Registration::Metric::MeanSquared4D metric;
      affine_registration.run_masked (metric, affine, moving_voxel, template_voxel, mmask_image, tmask_image);
    } else {
      Image::Registration::Metric::MeanSquared metric;
      affine_registration.run_masked (metric, affine, moving_voxel, template_voxel, mmask_image, tmask_image);
    }

    if (output_affine)
      affine.get_transform().save (affine_filename);
  }

  if (transformed_buffer_ptr) {
    Image::Buffer<float>::voxel_type transformed_voxel (*transformed_buffer_ptr);
    // if (do_rigid and ! do_affine) {
    //   Image::Filter::reslice<Image::Interp::Cubic> (moving_voxel, transformed_voxel, affine.get_transform(), Image::Adapter::AutoOverSample, 0.0);
    //   if (do_reorientation) {
    //     std::string msg ("reorienting...");
    //     Image::Registration::Transform::reorient (msg, transformed_voxel, transformed_voxel, affine.get_transform(), directions_cartesian);
    //   }
    if (do_affine && ! do_affine_sym) {
      Image::Filter::reslice<Image::Interp::Cubic> (moving_voxel, transformed_voxel, affine.get_transform(), Image::Adapter::AutoOverSample, 0.0);
      if (do_reorientation) {
        std::string msg ("reorienting... ");
        Image::Registration::Transform::reorient (msg, transformed_voxel, transformed_voxel, affine.get_transform(), directions_cartesian);
      }
    } else if (do_affine && do_affine_sym) {
      Image::Filter::reslice<Image::Interp::Cubic> (moving_voxel, transformed_voxel, affineSym.get_transform(), Image::Adapter::AutoOverSample, 0.0); 
      WARN ("symmetric reslice not implemented");
      if (do_reorientation) {
        std::string msg ("reorienting... ");
        Image::Registration::Transform::reorient (msg, transformed_voxel, transformed_voxel, affineSym.get_transform(), directions_cartesian);
      }
    } else {
      // moving_voxel.save("moving_voxel.mif");
      // transformed_voxel.save("transformed_voxel_before.mif");
      // Image::Filter::reslice<Image::Interp::Nearest> (moving_voxel, transformed_voxel, rigid.get_transform(), Image::Adapter::AutoOverSample, 0.0); 
      // transformed_voxel.save("transformed_voxel_nearest.mif");
      Image::Filter::reslice<Image::Interp::Cubic> (moving_voxel, transformed_voxel, rigid.get_transform(), Image::Adapter::AutoOverSample, 0.0); 
      // transformed_voxel.save("transformed_voxel_cubic.mif");
      // FIXME: Image::Interp::Cubic seems to produce integer overflow ,using Nearest instead


      // Image::Filter::reslice<Image::Interp::Linear> (source, destination, operation);
      // DataSet::Interp::reslice<DataSet::Interp::Linear> (destination, source);


      if (do_reorientation) {
        std::string msg ("reorienting... ");
        Image::Registration::Transform::reorient (msg, transformed_voxel, transformed_voxel, rigid.get_transform(), directions_cartesian);
      }
    }
  }
}
