/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert Smith, 2011.

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
#include "image/header.h"
#include "image/loop.h"
#include "image/nav.h"
#include "image/voxel.h"
#include "progressbar.h"

#include "dwi/tractography/ACT/act.h"
#include "dwi/tractography/ACT/tissues.h"


#define MIN_TISSUE_CHANGE 0.01 // Just accounting for floating-point errors



using namespace MR;
using namespace App;


void usage ()
{

	AUTHOR = "Robert E. Smith (r.smith@brain.org.au)";

	DESCRIPTION
	+ "Generate a mask image appropriate for seeding streamlines on the grey matter - white matter interface";

	REFERENCES = "Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. "
	             "Anatomically-constrained tractography: Improved diffusion MRI streamlines tractography through effective use of anatomical information. "
	             "NeuroImage, 2012, 62, 1924-1938";

	ARGUMENTS
	+ Argument ("5tt_in",  "the input 5TT segmented anatomical image").type_image_in()
	+ Argument ("mask_out", "the output mask image")                  .type_image_out();

	OPTIONS

	+ Option("mask_in", "Filter an input mask image according to those voxels that lie upon the grey matter - white matter boundary. \n"
	                    "If no input mask is provided, the output will be a whole-brain mask image calculated using the anatomical image only.")
	  + Argument ("image", "the input mask image").type_image_in();

}



void run ()
{

  Image::Header H_in (argument[0]);
  DWI::Tractography::ACT::verify_5TT_image (H_in);
  Image::Buffer<float> image_in (H_in);
  Image::Buffer<float>::voxel_type v_in (image_in);

  // TODO It would be nice to have the capability to define this mask based on another image
  // This will however require the use of interpolators

  Ptr< Image::Buffer<bool> > image_mask;
  Ptr< Image::Buffer<bool>::voxel_type > v_mask;
  Options opt = get_options ("mask_in");
  if (opt.size()) {
    image_mask = new Image::Buffer<bool> (opt[0][0]);
    if (!Image::dimensions_match (image_in, *image_mask, 0, 3))
      throw Exception ("Mask image provided using the -mask option must match the input 5TT image");
    v_mask = new Image::Buffer<bool>::voxel_type (*image_mask);
  }

  Image::Header H_out;
  if (image_mask) {
    H_out = *image_mask;
    H_out.datatype() = DataType::Float32;
    H_out.datatype().set_byte_order_native();
  } else {
    H_out = image_in;
    H_out.set_ndim (3);
  }
  Image::Buffer<float> image_out (argument[1], H_out);
  Image::Buffer<float>::voxel_type v_out (image_out);

  Image::LoopInOrder loop (v_out, "Determining GMWMI seeding mask...");
  for (auto l = loop (v_out); l; ++l) {

    // If a mask is defined, but is false in this voxel, do not continue processing
    bool process_voxel = true;
    if (v_mask) {
      (*v_mask)[2] = v_out[2];
      (*v_mask)[1] = v_out[1];
      (*v_mask)[0] = v_out[0];
      process_voxel = v_mask->value();
    }

    if (process_voxel) {

      // Generate a non-binary seeding mask.
      // Image intensity should be proportional to the tissue gradient present across the voxel
      // Remember: This seeding mask is generated in the same space as the 5TT image: exploit this
      //   (no interpolators should be necessary)
      // Essentially looking for an absolute gradient in (GM - WM) - just do in three axes
      //   - well, not quite; needs to be the minimum of the two
      float gradient = 0.0;
      for (size_t axis = 0; axis != 3; ++axis) {
        Image::Nav::set_pos (v_in, v_out, 0, 3);
        float multiplier = 0.5;
        if (!v_out[axis]) {
          multiplier = 1.0;
        } else {
          v_in[axis] = v_out[axis] - 1;
        }
        const DWI::Tractography::ACT::Tissues neg (v_in);
        if (v_out[axis] == v_out.dim(axis)-1) {
          multiplier = 1.0;
          v_in[axis] = v_out[axis];
        } else {
          v_in[axis] = v_out[axis] + 1;
        }
        const DWI::Tractography::ACT::Tissues pos (v_in);
        gradient += Math::pow2 (multiplier * std::min (std::abs (pos.get_gm() - neg.get_gm()), std::abs (pos.get_wm() - neg.get_wm())));
      }
      gradient = std::sqrt (gradient);
      v_out.value() = gradient;

    } else {
      v_out.value() = 0.0f;
    }

  }

}

