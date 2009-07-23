/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

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


    29-10-2008 J-Donald Tournier <d.tournier@brain.org.au>
    * fix init_direction handling

    18-03-2009 J-Donald Tournier <d.tournier@brain.org.au>
    * fix serious bug that caused the tracking to be incorrect with obliquely aligned data sets

*/

#include "dwi/tractography/tracker/base.h"

namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Tracker {

        Base::Base (Image::Object& source_image, Properties& properties) :
          source (source_image),
          props (properties),
          total_seed_volume (0.0),
          step_size (0.1),
          threshold (0.1),
          num_points (0)
        {
          if (props["step_size"].empty()) props["step_size"] = str (step_size); step_size = to<float> (props["step_size"]); 
          if (props["threshold"].empty()) props["threshold"] = str (threshold); else threshold = to<float> (props["threshold"]); 
          if (props["init_threshold"].empty()) { init_threshold = 2.0*threshold; props["init_threshold"] = str (init_threshold); }
          else init_threshold = to<float> (props["init_threshold"]); 

          float max_dist = 200.0;
          if (props["max_dist"].empty()) props["max_dist"] = str(max_dist); else max_dist = to<float> (props["max_dist"]);
          num_max = round (max_dist/step_size);
        
          props["source"] = source.name();

          for (std::vector<RefPtr<ROI> >::iterator i = properties.roi.begin(); i != properties.roi.end(); ++i) {
            ROI& roi (**i);
            if (!roi.mask.empty() && !roi.mask_object) {
              roi.mask_object = new MR::Image::Object;
              roi.mask_object->open (roi.mask);
            }
            switch (roi.type) {
              case ROI::Seed:
                if (roi.mask.empty()) spheres.seed.push_back (Sphere (roi.position, roi.radius));
                else masks.seed.push_back (Mask (*roi.mask_object)); 
                break;
              case ROI::Include:
                if (roi.mask.empty()) spheres.include.push_back (Sphere (roi.position, roi.radius));
                else masks.include.push_back (Mask (*roi.mask_object));
                break;
              case ROI::Exclude:
                if (roi.mask.empty()) spheres.exclude.push_back (Sphere (roi.position, roi.radius));
                else masks.exclude.push_back (Mask (*roi.mask_object));
                break;
              case ROI::Mask:
                if (roi.mask.empty()) spheres.mask.push_back (Sphere (roi.position, roi.radius));
                else masks.mask.push_back (Mask (*roi.mask_object));
                break;
              default: assert (0);
            }
          }

          if (spheres.seed.empty() && masks.seed.empty()) throw Exception ("no seed region specified!");

          for (std::vector<Sphere>::const_iterator i = spheres.seed.begin(); i != spheres.seed.end(); ++i) total_seed_volume += i->volume; 
          for (std::vector<Mask>::const_iterator i = masks.seed.begin(); i != masks.seed.end(); ++i) total_seed_volume += i->volume;
        }



        Base::~Base () { }




        void Base::new_seed (const Point& seed_dir)
        {
          excluded = false;
          for (std::vector<Sphere>::iterator i = spheres.include.begin(); i != spheres.include.end(); ++i) i->included = false;
          for (std::vector<Mask>::iterator i = masks.include.begin(); i != masks.include.end(); ++i) i->included = false;

          Point seed_point;
          do { seed_point = gen_seed(); } while (not_in_mask (seed_point));
          set (seed_point, seed_dir);
        }





        bool Base::next () 
        {
          if (excluded) return (false);
          if (num_points >= num_max) return (false);
          if (not_in_mask (pos)) return (false);
          if (next_point()) return (false);
          num_points++;

          for (std::vector<Sphere>::iterator i = spheres.exclude.begin(); i != spheres.exclude.end(); ++i) { if (i->contains (pos)) { excluded = true; return (false); } }
          for (std::vector<Mask>::iterator i = masks.exclude.begin(); i != masks.exclude.end(); ++i) { if (i->contains (pos)) { excluded = true; return (false); } }

          for (std::vector<Sphere>::iterator i = spheres.include.begin(); i != spheres.include.end(); ++i) if (!i->included) if (i->contains (pos)) i->included = true; 
          for (std::vector<Mask>::iterator i = masks.include.begin(); i != masks.include.end(); ++i) if (!i->included) if (i->contains (pos)) i->included = true;

          return (true);
        }



      }
    }
  }
}
