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


#include "dwi/tractography/editing/editing.h"


namespace MR {
namespace DWI {
namespace Tractography {
namespace Editing {



using namespace App;





const OptionGroup LengthOption = OptionGroup ("Streamline length threshold options")

  + Option ("maxlength", "set the maximum length of any streamline in mm")
    + Argument ("value").type_float (0.0)

  + Option ("minlength", "set the minimum length of any streamline in mm")
    + Argument ("value").type_float (0.0);






const OptionGroup TruncateOption = OptionGroup ("Streamline count truncation options")

  + Option ("number", "set the desired number of selected streamlines to be propagated to the output file")
    + Argument ("count").type_integer (1)

  + Option ("skip", "omit this number of selected streamlines before commencing writing to the output file")
    + Argument ("count").type_integer (1);





const OptionGroup WeightsOption = OptionGroup ("Thresholds pertaining to per-streamline weighting")

  + Option ("maxweight", "set the maximum weight of any streamline")
    + Argument ("value").type_float (0.0)

  + Option ("minweight", "set the minimum weight of any streamline")
    + Argument ("value").type_float (0.0);










void load_properties (Tractography::Properties& properties)
{

  // LengthOption
  auto opt = get_options ("maxlength");
  if (opt.size()) {
    if (properties.find ("max_dist") == properties.end()) {
      properties["max_dist"] = str(float (opt[0][0]));
    } else {
      try {
        const float maxlength = std::min (float(opt[0][0]), to<float>(properties["max_dist"]));
        properties["max_dist"] = str(maxlength);
      } catch (...) {
        properties["max_dist"] = str(float (opt[0][0]));
      }
    }
  }
  opt = get_options ("minlength");
  if (opt.size()) {
    if (properties.find ("min_dist") == properties.end()) {
      properties["min_dist"] = str(float (opt[0][0]));
    } else {
      try {
        const float minlength = std::max (float(opt[0][0]), to<float>(properties["min_dist"]));
        properties["min_dist"] = str(minlength);
      } catch (...) {
        properties["min_dist"] = str(float (opt[0][0]));
      }
    }
  }


  // TruncateOption
  // These have no influence on Properties


  // WeightsOption
  // Only the thresholds have an influence on Properties
  opt = get_options ("maxweight");
  if (opt.size())
    properties["max_weight"] = str (float (opt[0][0]));
  opt = get_options ("minweight");
  if (opt.size())
    properties["min_weight"] = str (float (opt[0][0]));

}




}
}
}
}
