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
#include "image/interp/linear.h"
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
  Image::Interp::Linear< Image::Buffer<float>::voxel_type > interp_in (v_in);

  Ptr< Image::Buffer<bool> > image_mask;
  Ptr< Image::Buffer<bool>::voxel_type > v_mask;
  Options opt = get_options ("mask_in");
  if (opt.size()) {
    image_mask = new Image::Buffer<bool> (opt[0][0]);
    v_mask = new Image::Buffer<bool>::voxel_type (*image_mask);
  }

  Image::Header H_out;
  if (image_mask) {
    H_out = *image_mask;
  } else {
    H_out = image_in;
    H_out.set_ndim (3);
    H_out.datatype() = DataType::Bit;
  }
  Image::Buffer<bool> image_out (argument[1], H_out);
  Image::Buffer<bool>::voxel_type v_out (image_out);
  Image::Interp::Linear< Image::Buffer<bool>::voxel_type > interp_out (v_out);

  Image::Loop loop ("Determining GMWMI seeding mask...");
  for (loop.start (v_out); loop.ok(); loop.next (v_out)) {

    // If a mask is defined, but is false in this voxel, do not continue processing
    bool process_voxel = true;
    if (v_mask) {
      (*v_mask)[2] = v_out[2];
      (*v_mask)[1] = v_out[1];
      (*v_mask)[0] = v_out[0];
      process_voxel = v_mask->value();
    }

    if (process_voxel) {

      // Determine whether or not this voxel should be true in the output mask
      // Base this decision on the presence of a gradient within the voxel volume of both the GM and the WM fractions

      const Point<float> voxel_centre   (v_out[0], v_out[1], v_out[2]);
      const Point<float> p_voxel_centre (interp_out.voxel2scanner (voxel_centre));

      if (!interp_in.scanner (p_voxel_centre)) {

        DWI::Tractography::ACT::Tissues values_voxel_centre (interp_in);
        if (values_voxel_centre.valid()) {

          bool mask_value = false;

          for (size_t i = 0; !mask_value && i != 8; ++i) {
            Point<float> voxel_corner;
            switch (i) {
              case 0: voxel_corner.set (v_out[0] - 0.5, v_out[1] - 0.5, v_out[2] - 0.5); break;
              case 1: voxel_corner.set (v_out[0] - 0.5, v_out[1] - 0.5, v_out[2] + 0.5); break;
              case 2: voxel_corner.set (v_out[0] - 0.5, v_out[1] + 0.5, v_out[2] - 0.5); break;
              case 3: voxel_corner.set (v_out[0] - 0.5, v_out[1] + 0.5, v_out[2] + 0.5); break;
              case 4: voxel_corner.set (v_out[0] + 0.5, v_out[1] - 0.5, v_out[2] - 0.5); break;
              case 5: voxel_corner.set (v_out[0] + 0.5, v_out[1] - 0.5, v_out[2] + 0.5); break;
              case 6: voxel_corner.set (v_out[0] + 0.5, v_out[1] + 0.5, v_out[2] - 0.5); break;
              case 7: voxel_corner.set (v_out[0] + 0.5, v_out[1] + 0.5, v_out[2] + 0.5); break;
            }
            const Point<float> p_voxel_corner (interp_out.voxel2scanner (voxel_corner));
            if (!interp_in.scanner (p_voxel_corner)) {
              DWI::Tractography::ACT::Tissues values_voxel_corner (interp_in);
              if (values_voxel_corner.valid()) {

                // No absolute values for differences here - one must increase, the other must decrease
                mask_value = ((values_voxel_corner.get_gm() - values_voxel_centre.get_gm() > MIN_TISSUE_CHANGE) && (values_voxel_centre.get_wm() - values_voxel_corner.get_wm() > MIN_TISSUE_CHANGE))
                          || ((values_voxel_centre.get_gm() - values_voxel_corner.get_gm() > MIN_TISSUE_CHANGE) && (values_voxel_corner.get_wm() - values_voxel_centre.get_wm() > MIN_TISSUE_CHANGE));


              }

            }

          }

          v_out.value() = mask_value;

        }

      }

    }

  }

}

