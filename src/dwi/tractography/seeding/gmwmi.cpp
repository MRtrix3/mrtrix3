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
#include "dwi/tractography/rng.h"

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
        perturb_max_step (4.0f * std::pow (anat_data.voxsize(0) * anat_data.voxsize(1) * anat_data.voxsize(2), (1.0f/3.0f)))
      {
        volume = init_seeder.vol();
      }




      bool GMWMI::get_seed (Eigen::Vector3f& p) const
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



      bool GMWMI::perturb (Eigen::Vector3f& p, Interp& interp) const
      {
        const auto normal = get_normal (p, interp);
        if (!normal.allFinite())
          return false;
        Eigen::Vector3f plane_one (normal.cross (Eigen::Vector3f(0.0f, 0.0f, 1.0f)).normalized());
        if (!plane_one.allFinite())
          plane_one = (normal.cross (Eigen::Vector3f (0.0f,1.0f,0.0f))).normalized();
        const Eigen::Vector3f plane_two ((normal.cross (plane_one)).normalized());
        std::uniform_real_distribution<float> uniform;
        p += ((uniform(rng)-0.5f) * perturb_max_step * plane_one) + ((uniform(rng)-0.5f) * perturb_max_step * plane_two);
        return find_interface (p, interp);
      }




      }
    }
  }
}


