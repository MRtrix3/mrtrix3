/* Copyright (c) 2008-2023 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#ifndef __testing_diff_images_h__
#define __testing_diff_images_h__

#include "progressbar.h"
#include "datatype.h"

#include "image.h"
#include "image_helpers.h"
#include "image_diff.h"

#include "adapter/replicate.h"

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
      + App::Option ("image", "specify an image containing the tolerances")
        + App::Argument ("path").type_image_in()
      + App::Option ("voxel", "specify a fractional tolerance relative to the maximum value in the voxel")
        + App::Argument ("tolerance").type_float (0.0);



    template <class ImageType1, class ImageType2>
    void diff_images (ImageType1& in1, ImageType2& in2)
    {

      auto abs_opt = App::get_options ("abs");
      auto frac_opt = App::get_options ("frac");
      auto image_opt = App::get_options ("image");
      auto voxel_opt = App::get_options ("voxel");

      if (abs_opt.size()){
        check_images_abs (in1, in2, abs_opt[0][0]);
      } else if (frac_opt.size()) {
        check_images_frac (in1, in2, frac_opt[0][0]);
      } else if (image_opt.size()) {
        auto tolerance = Image<default_type>::open (image_opt[0][0]);
        Adapter::Replicate<decltype(tolerance)> replicate (tolerance, in1);
        check_images_tolimage (in1, in2, replicate);
      } else if (voxel_opt.size()) {
        check_images_voxel (in1, in2, voxel_opt[0][0]);
      } else {
        check_images_abs (in1, in2, 0.0);
      }

    }
  }
}

#endif
