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
    + Argument ("ratio").type_integer (1, 1, 1e6);





const OptionGroup TruncateOption = OptionGroup ("Streamline count truncation options")

  + Option ("number", "set the desired number of selected streamlines to be propagated to the output file")
    + Argument ("count").type_integer (0, 0, std::numeric_limits<int>::max())

  + Option ("skip", "omit this number of selected streamlines before commencing writing to the output file")
    + Argument ("count").type_integer (0, 0, std::numeric_limits<int>::max());





const OptionGroup WeightsOption = OptionGroup ("Thresholds pertaining to per-streamline weighting")

  + Option ("maxweight", "set the maximum weight of any streamline")
    + Argument ("value").type_float (0.0, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity())

  + Option ("minlength", "set the minimum weight of any streamline")
    + Argument ("value").type_float (0.0, 0.0, std::numeric_limits<float>::infinity());










void load_properties (Tractography::Properties& properties)
{

  // ROIOption
  Options opt = get_options ("include");
  for (size_t i = 0; i < opt.size(); ++i)
    properties.include.add (ROI (opt[i][0]));

  opt = get_options ("exclude");
  for (size_t i = 0; i < opt.size(); ++i)
    properties.exclude.add (ROI (opt[i][0]));

  opt = get_options ("mask");
  for (size_t i = 0; i < opt.size(); ++i)
    properties.mask.add (ROI (opt[i][0]));


  // LengthOption
  // Erase entries from properties if these criteria are not being applied here
  // TODO Technically shouldn't be required - shouldn't be any streamlines outside the range in properties
  opt = get_options ("maxlength");
  float maxlength = (properties.find ("max_dist") == properties.end()) ? float(0.0) : to<float>(properties["max_dist"]);
  if (opt.size()) {
    maxlength = maxlength ? std::min (maxlength, float(opt[0][0])) : float(opt[0][0]);
    properties["max_dist"] = str(maxlength);
  } else if (maxlength) {
    properties.erase (properties.find ("max_dist"));
  }

  opt = get_options ("minlength");
  float minlength = (properties.find ("min_dist") == properties.end()) ? float(0.0) : to<float>(properties["min_dist"]);
  if (opt.size()) {
    minlength = minlength ? std::max (minlength, float(opt[0][0])) : float(opt[0][0]);
    properties["min_dist"] = str(minlength);
  } else if (minlength) {
    properties.erase (properties.find ("min_dist"));
  }


  // ResampleOption
  // The relevant entry in Properties is updated at a later stage


  // TruncateOption
  // These have no influence on Properties


  // WeightsOption
  // Only the thresholds have an influence on Properties
  opt = get_options ("maxweight");
  if (opt.size())
    properties["maxweight"] = std::string(opt[0][0]);
  opt = get_options ("minweight");
  if (opt.size())
    properties["minweight"] = std::string(opt[0][0]);

}




}
}
}
}
