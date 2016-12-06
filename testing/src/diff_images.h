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


#ifndef __testing_diff_images_h__
#define __testing_diff_images_h__

#include "progressbar.h"
#include "datatype.h"

#include "image.h"
#include "image_helpers.h"
#include "image_diff.h"

namespace MR
{
  namespace Testing
  {


    const App::OptionGroup Diff_Image_Options =
      App::OptionGroup ("Testing image options")
      + App::Option ("abs", "specify an absolute tolerance")
        + App::Argument ("tolerance").type_float (0.0)
      + App::Option ("frac", "specify a fractional tolerance")
        + App::Argument ("tolerance").type_float (0.0)
      + App::Option ("voxel", "specify a fractional tolerance relative to the maximum value in the voxel")
        + App::Argument ("tolerance").type_float (0.0);



    template <class ImageType1, class ImageType2>
    void diff_images (ImageType1& in1, ImageType2& in2)
    {

      double tolerance = 0.0;
      auto abs_opt = App::get_options ("abs");
      auto frac_opt = App::get_options ("frac");
      auto voxel_opt = App::get_options ("voxel");

      if (frac_opt.size()) {
        tolerance = frac_opt[0][0];
        check_images_frac (in1, in2, tolerance);
      } else if (voxel_opt.size()) {
        tolerance = voxel_opt[0][0];
        check_images_voxel (in1, in2, tolerance);
      } else if (abs_opt.size()){
        tolerance = abs_opt[0][0];
        check_images_abs (in1, in2, tolerance);
      } else {
        check_images_abs (in1, in2, tolerance);
      }

    }
  }
}

#endif
