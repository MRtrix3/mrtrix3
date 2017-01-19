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
#include "image.h"
#include "algo/copy.h"
#include "adapter/subset.h"



using namespace MR;
using namespace App;



void usage ()
{

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  DESCRIPTION
    + "Crop an image series to a reduced field of view, using either manual setting of axis dimensions, or a computed mask image corresponding to the brain. \n"
    + "If using a mask, a gap of 1 voxel will be left at all 6 edges of the image such that trilinear interpolation upon the resulting images is still valid. \n"
    + "This is useful for axially-acquired brain images, where the image size can be reduced by a factor of 2 by removing the empty space on either side of the brain.";

  ARGUMENTS
    + Argument ("image_in",  "the image to be cropped").type_image_in()
    + Argument ("image_out", "the output path for the resulting cropped image").type_image_out();


  OPTIONS

    + Option   ("mask",  "crop the input image according to the spatial extent of a mask image")
    + Argument ("image", "the mask image").type_image_in()

    + Option   ("axis",  "crop the input image in the provided axis").allow_multiple()
    + Argument ("index", "the index of the image axis to be cropped").type_integer (0, 2)
    + Argument ("start", "the first voxel along this axis to be included in the output image").type_integer (0)
    + Argument ("end",   "the last voxel along this axis to be included in the output image").type_integer (0);

}




void run ()
{

  auto in = Image<float>::open (argument[0]);

  vector<vector<ssize_t>> bounds (in.ndim(), vector<ssize_t> (2));
  for (size_t axis = 0; axis < in.ndim(); axis++) {
    bounds[axis][0] = 0;
    bounds[axis][1] = in.size (axis) - 1;
  }

  auto opt = get_options ("mask");
  if (opt.size()) {

    auto mask = Image<bool>::open (opt[0][0]);
    check_dimensions (in, mask, 0, 3);

    for (size_t axis = 0; axis != 3; ++axis) {
      bounds[axis][0] = in.size (axis);
      bounds[axis][1] = 0;
    }

    struct BoundsCheck { NOMEMALIGN
      vector<vector<ssize_t>>& overall_bounds;
      vector<vector<ssize_t>> bounds;
      BoundsCheck (vector<vector<ssize_t>>& overall_bounds) : overall_bounds (overall_bounds), bounds (overall_bounds) { }
      ~BoundsCheck () {
        for (size_t axis = 0; axis != 3; ++axis) {
          overall_bounds[axis][0] = std::min (bounds[axis][0], overall_bounds[axis][0]);
          overall_bounds[axis][1] = std::max (bounds[axis][1], overall_bounds[axis][1]);
        }
      }
      void operator() (const decltype(mask)& m) { 
        if (m.value()) {
          for (size_t axis = 0; axis != 3; ++axis) {
            bounds[axis][0] = std::min (bounds[axis][0], m.index(axis));
            bounds[axis][1] = std::max (bounds[axis][1], m.index(axis));
          }
        }
      }
    };

    // Note that even though only 3 dimensions are cropped when using a mask, the bounds
    // are computed by checking the extent for all dimensions (for example a 4D AFD mask)
    ThreadedLoop (mask).run (BoundsCheck (bounds), mask);

    for (size_t axis = 0; axis != 3; ++axis) {
      if (bounds[axis][0] > bounds[axis][1])
        throw Exception ("mask image is empty; can't use to crop image");
      if (bounds[axis][0])
        --bounds[axis][0];
      if (bounds[axis][1] < mask.size (axis) - 1)
        ++bounds[axis][1];
    }

  }

  opt = get_options ("axis");
  for (size_t i = 0; i != opt.size(); ++i) {
    // Manual cropping of axis overrides mask image bounds
    const ssize_t axis  = opt[i][0];
    const ssize_t start = opt[i][1];
    const ssize_t end   = opt[i][2];
    if (start < 0 || end >= in.size(axis))
      throw Exception ("Index supplied for axis " + str(axis) + " is out of bounds");
    if (end < start)
      throw Exception  ("End index supplied for axis " + str(axis) + " is less than start index");
    bounds[axis][0] = start;
    bounds[axis][1] = end;
  }

  vector<size_t> from (in.ndim());
  vector<size_t> size (in.ndim());
  for (size_t axis = 0; axis < in.ndim(); axis++) {
    from[axis] = bounds[axis][0];
    size[axis] = bounds[axis][1] - from[axis] + 1;
  }

  auto cropped = Adapter::make<Adapter::Subset> (in, from, size);
  auto out = Image<float>::create (argument[1], cropped);
  threaded_copy_with_progress_message ("cropping image...", cropped, out);
}

