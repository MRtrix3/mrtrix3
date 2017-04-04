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

#include "dwi/tractography/resampling/resampling.h"

#include "dwi/tractography/resampling/arc.h"
#include "dwi/tractography/resampling/downsampler.h"
#include "dwi/tractography/resampling/endpoints.h"
#include "dwi/tractography/resampling/fixed_num_points.h"
#include "dwi/tractography/resampling/fixed_step_size.h"
#include "dwi/tractography/resampling/upsampler.h"



namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Resampling {



        using namespace App;

        const OptionGroup ResampleOption = OptionGroup ("Streamline resampling options")

            + Option ("upsample", "increase the density of points along the length of each streamline by some factor "
                                  "(may improve mapping streamlines to ROIs, and/or visualisation)")
              + Argument ("ratio").type_integer (1)

            + Option ("downsample", "increase the density of points along the length of each streamline by some factor "
                                    "(decreases required storage space)")
              + Argument ("ratio").type_integer (1)

            + Option ("step_size", "re-sample the streamlines to a desired step size (in mm)")
              + Argument ("value").type_float (0.0)

            + Option ("num_points", "re-sample each streamline to a fixed number of points")
              + Argument ("count").type_integer (2)

            + Option ("endpoints", "only output the two endpoints of each streamline")

            + Option ("line",
                      "resample tracks at 'num' equidistant locations "
                      "along a line between 'start' and 'end' (specified as "
                      "comma-separated 3-vectors in scanner coordinates)")
              + Argument ("num").type_integer (2)
              + Argument ("start").type_sequence_float()
              + Argument ("end").type_sequence_float()

            + Option ("arc",
                      "resample tracks at 'num' equidistant locations "
                      "along a circular arc specified by points 'start', 'mid' and 'end' "
                      "(specified as comma-separated 3-vectors in scanner coordinates)")
              + Argument ("num").type_integer (2)
              + Argument ("start").type_sequence_float()
              + Argument ("mid").type_sequence_float()
              + Argument ("end").type_sequence_float();



        namespace {
          using value_type = float;
          using point_type = Eigen::Vector3f;
          point_type get_pos (const std::vector<default_type>& s)
          {
            if (s.size() != 3)
              throw Exception ("position expected as a comma-seperated list of 3 values");
            return { value_type(s[0]), value_type(s[1]), value_type(s[2]) };
          }
        }



        Base* get_resampler()
        {
          const size_t count = (get_options ("upsample").size() ? 1 : 0) +
                               (get_options ("downsample").size() ? 1 : 0) +
                               (get_options ("step_size").size() ? 1 : 0) +
                               (get_options ("num_points").size() ? 1 : 0) +
                               (get_options ("endpoints").size() ? 1 : 0) +
                               (get_options ("line").size() ? 1 : 0) +
                               (get_options ("arc").size() ? 1 : 0);
          if (!count)
            throw Exception ("Must specify a mechanism for resampling streamlines");
          if (count > 1)
            throw Exception ("Can only use one form of streamline resampling");

          auto opt = get_options ("upsample");
          if (opt.size())
            return new Upsampler (opt[0][0]);
          opt = get_options ("downsample");
          if (opt.size())
            return new Downsampler (opt[0][0]);
          opt = get_options ("step_size");
          if (opt.size())
            return new FixedStepSize (opt[0][0]);
          opt = get_options ("num_points");
          if (opt.size())
            return new FixedNumPoints (opt[0][0]);
          opt = get_options ("endpoints");
          if (opt.size())
            return new Endpoints;
          opt = get_options ("line");
          if (opt.size())
            return new Arc (opt[0][0],
                            get_pos (opt[0][1].as_sequence_float()),
                            get_pos (opt[0][2].as_sequence_float()));
          opt = get_options ("arc");
          if (opt.size())
            return new Arc (opt[0][0],
                            get_pos (opt[0][1].as_sequence_float()),
                            get_pos (opt[0][2].as_sequence_float()),
                            get_pos (opt[0][3].as_sequence_float()));

          assert (0);
          return nullptr;
        }



      }
    }
  }
}


