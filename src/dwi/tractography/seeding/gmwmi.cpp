/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 2012.

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


#include "dwi/tractography/seeding/gmwmi.h"



namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Seeding
      {




      GMWMI::GMWMI (const std::string& in, const std::string& anat_path) :
        Base (in, "GM-WM interface", MAX_TRACKING_SEED_ATTEMPTS_GMWMI),
        GMWMI_5TT_Wrapper (anat_path),
        ACT::GMWMI_finder (anat_data),
        init_seeder (in),
        perturb_max_step (4.0f * std::pow (anat_data.vox(0) * anat_data.vox(1) * anat_data.vox(2), (1.0f/3.0f)))
      {
        volume = init_seeder.vol();
      }




      bool GMWMI::get_seed (Point<float>& p)
      {
        Interp interp (interp_template);
        do {
          init_seeder.get_seed (p);
          if (find_interface (p, interp)) {
            if (perturb (p, interp))
              return true;
          }
        } while (1);
        return false;
      }



      bool GMWMI::perturb (Point<float>& p, Interp& interp)
      {
        const Point<float> normal (get_normal (p, interp));
        if (!normal.valid())
          return false;
        Point<float> plane_one ((normal.cross (Point<float> (0.0f,0.0f,1.0f))).normalise());
        if (!plane_one.valid())
          plane_one = (normal.cross (Point<float> (0.0f,1.0f,0.0f))).normalise();
        const Point<float> plane_two ((normal.cross (plane_one)).normalise());
        p += ((rng()-0.5f) * perturb_max_step * plane_one) + ((rng()-0.5f) * perturb_max_step * plane_two);
        return find_interface (p, interp);
      }




      }
    }
  }
}


