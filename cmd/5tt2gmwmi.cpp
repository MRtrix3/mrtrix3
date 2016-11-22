/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */







#include "command.h"
#include "header.h"
#include "image.h"
#include "image_helpers.h"
#include "progressbar.h"

#include "algo/loop.h"

#include "dwi/tractography/ACT/act.h"
#include "dwi/tractography/ACT/tissues.h"


#define MIN_TISSUE_CHANGE 0.01 // Just accounting for floating-point errors



using namespace MR;
using namespace App;


void usage ()
{

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  DESCRIPTION
    + "Generate a mask image appropriate for seeding streamlines on the grey matter - white matter interface";

  REFERENCES
    + "Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. " // Internal
      "Anatomically-constrained tractography:"
      "Improved diffusion MRI streamlines tractography through effective use of anatomical information. "
      "NeuroImage, 2012, 62, 1924-1938";

  ARGUMENTS
    + Argument ("5tt_in",  "the input 5TT segmented anatomical image").type_image_in()
    + Argument ("mask_out", "the output mask image")                  .type_image_out();

  OPTIONS
    + Option("mask_in", "Filter an input mask image according to those voxels that lie upon the grey matter - white matter boundary. \n"
                        "If no input mask is provided, the output will be a whole-brain mask image calculated using the anatomical image only.")
      + Argument ("image", "the input mask image").type_image_in();

}



class Processor
{ MEMALIGN(Processor)

  public:
    Processor (const Image<bool>& mask) : mask (mask) { }
    Processor (const Processor&) = default;

    bool operator() (Image<float>& input, Image<float>& output)
    {
      // If a mask is defined, but is false in this voxel, do not continue processing
      bool process_voxel = true;
      if (mask.valid()) {
        assign_pos_of (input, 0, 3).to (mask);
        process_voxel = mask.value();
      }
      if (process_voxel) {
        // Generate a non-binary seeding mask.
        // Image intensity should be proportional to the tissue gradient present across the voxel
        // Remember: This seeding mask is generated in the same space as the 5TT image: exploit this
        //   (no interpolators should be necessary)
        // Essentially looking for an absolute gradient in (GM - WM) - just do in three axes
        //   - well, not quite; needs to be the minimum of the two
        default_type gradient = 0.0;
        for (size_t axis = 0; axis != 3; ++axis) {
          assign_pos_of (output, 0, 3).to (input);
          default_type multiplier = 0.5;
          if (!output.index(axis)) {
            multiplier = 1.0;
          } else {
            input.move_index (axis, -1);
          }
          const DWI::Tractography::ACT::Tissues neg (input);
          if (output.index(axis) == output.size(axis)-1) {
            multiplier = 1.0;
            input.index(axis) = output.index(axis);
          } else {
            input.index(axis) = output.index(axis) + 1;
          }
          const DWI::Tractography::ACT::Tissues pos (input);
          gradient += Math::pow2 (multiplier * std::min (std::abs (pos.get_gm() - neg.get_gm()), std::abs (pos.get_wm() - neg.get_wm())));
        }
        output.value() = std::max (0.0, std::sqrt (gradient));
        assign_pos_of (output, 0, 3).to (input);
      } else {
        output.value() = 0.0f;
      }
      return true;
    }

  private:
    Image<bool> mask;

};



void run ()
{

  auto input = Image<float>::open (argument[0]);
  DWI::Tractography::ACT::verify_5TT_image (input);

  // TODO It would be nice to have the capability to define this mask based on another image
  // This will however require the use of interpolators

  Image<bool> mask;
  auto opt = get_options ("mask_in");
  if (opt.size()) {
    mask.open (opt[0][0]);
    if (!dimensions_match (input, mask, 0, 3))
      throw Exception ("Mask image provided using the -mask option must match the input 5TT image");
  }

  Header H;
  if (mask.valid()) {
    H = mask;
    H.datatype() = DataType::Float32;
    H.datatype().set_byte_order_native();
  } else {
    H = input;
    H.ndim() = 3;
  }
  auto output = Image<float>::create (argument[1], H);

  ThreadedLoop ("Generating GMWMI seed mask", input, 0, 3)
    .run (Processor (mask), input, output);

}

