#include "dwi/tractography/ACT/act.h"
#include "dwi/tractography/properties.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace ACT
      {

        using namespace App;

        const OptionGroup ACTOption = OptionGroup ("Anatomically-Constrained Tractography options")

          + Option ("act", "use the Anatomically-Constrained Tractography framework;\n"
                           "provided image must be in the 4TT (four-tissue-type) format")
            + Argument ("image").type_image_in()

          + Option ("backtrack", "allow tracks to be truncated and re-tracked if a poor structural termination is encountered")

          + Option ("crop_at_gmwmi", "crop streamline endpoints more precisely as they cross the GM-WM interface");



        void load_act_properties (Properties& properties)
        {

          using namespace MR::App;

          Options opt = get_options ("act");
          if (opt.size()) {

            properties["act"] = std::string (opt[0][0]);
            opt = get_options ("backtrack");
            if (opt.size())
              properties["backtrack"] = "1";
            opt = get_options ("crop_at_gmwmi");
            if (opt.size())
              properties["crop_at_gmwmi"] = "1";

          } else {

            if (get_options ("backtrack").size())
              WARN ("ignoring -backtrack option - only valid if using ACT");
            if (get_options ("crop_at_gmwmi").size())
              WARN ("ignoring -crop_at_gmwmi option - only valid if using ACT");

          }

        }


      }
    }
  }
}


