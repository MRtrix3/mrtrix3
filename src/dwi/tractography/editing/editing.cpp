/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 2014.

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


#include "dwi/tractography/editing/editing.h"


namespace MR {
namespace DWI {
namespace Tractography {
namespace Editing {



using namespace App;





const OptionGroup LengthOption = OptionGroup ("Streamline length threshold options")

  + Option ("maxlength", "set the maximum length of any streamline in mm")
    + Argument ("value").type_float (0.0, 0.0, std::numeric_limits<float>::infinity())

  + Option ("minlength", "set the minimum length of any streamline in mm")
    + Argument ("value").type_float (0.0, 0.0, std::numeric_limits<float>::infinity());






const OptionGroup ResampleOption = OptionGroup ("Streamline resampling options")

  + Option ("upsample", "increase the density of points along the length of the streamline by some factor "
                        "(may improve mapping streamlines to ROIs, and/or visualisation)")
    + Argument ("ratio").type_integer (1, 1, 1e6)

  + Option ("downsample", "increase the density of points along the length of the streamline by some factor "
                          "(decreases required storage space)")
    + Argument ("ratio").type_integer (1, 1, 1e6)

  + Option ("out_ends_only", "only output the two endpoints of each streamline");





const OptionGroup TruncateOption = OptionGroup ("Streamline count truncation options")

  + Option ("number", "set the desired number of selected streamlines to be propagated to the output file")
    + Argument ("count").type_integer (0, 0, std::numeric_limits<int>::max())

  + Option ("skip", "omit this number of selected streamlines before commencing writing to the output file")
    + Argument ("count").type_integer (0, 0, std::numeric_limits<int>::max());





const OptionGroup WeightsOption = OptionGroup ("Thresholds pertaining to per-streamline weighting")

  + Option ("maxweight", "set the maximum weight of any streamline")
    + Argument ("value").type_float (0.0, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity())

  + Option ("minweight", "set the minimum weight of any streamline")
    + Argument ("value").type_float (0.0, 0.0, std::numeric_limits<float>::infinity());










void load_properties (Tractography::Properties& properties)
{

  // ROIOption
  auto opt = get_options ("include");
  for (size_t i = 0; i < opt.size(); ++i)
    properties.include.add (ROI (opt[i][0]));

  opt = get_options ("exclude");
  for (size_t i = 0; i < opt.size(); ++i)
    properties.exclude.add (ROI (opt[i][0]));

  opt = get_options ("mask");
  for (size_t i = 0; i < opt.size(); ++i)
    properties.mask.add (ROI (opt[i][0]));


  // LengthOption
  opt = get_options ("maxlength");
  if (opt.size()) {
    if (properties.find ("max_dist") == properties.end()) {
      properties["max_dist"] = str(opt[0][0]);
    } else {
      try {
        const float maxlength = std::min (float(opt[0][0]), to<float>(properties["max_dist"]));
        properties["max_dist"] = str(maxlength);
      } catch (...) {
        properties["max_dist"] = str(opt[0][0]);
      }
    }
  }
  opt = get_options ("minlength");
  if (opt.size()) {
    if (properties.find ("min_dist") == properties.end()) {
      properties["min_dist"] = str(opt[0][0]);
    } else {
      try {
        const float minlength = std::max (float(opt[0][0]), to<float>(properties["min_dist"]));
        properties["min_dist"] = str(minlength);
      } catch (...) {
        properties["min_dist"] = str(opt[0][0]);
      }
    }
  }


  // ResampleOption
  // The relevant entry in Properties is updated at a later stage


  // TruncateOption
  // These have no influence on Properties


  // WeightsOption
  // Only the thresholds have an influence on Properties
  opt = get_options ("maxweight");
  if (opt.size())
    properties["max_weight"] = std::string(opt[0][0]);
  opt = get_options ("minweight");
  if (opt.size())
    properties["min_weight"] = std::string(opt[0][0]);

}




}
}
}
}
