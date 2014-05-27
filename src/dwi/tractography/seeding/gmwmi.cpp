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




      GMWMI::GMWMI (const std::string& in, const Math::RNG& rng, const std::string& anat_path) :
        Base (in, rng, "GM-WM interface", MAX_TRACKING_SEED_ATTEMPTS_GMWMI),
        init_seeder (in, rng),
        anat_data (anat_path),
        interface (anat_data)
      {
        volume = init_seeder.vol();
      }




      bool GMWMI::get_seed (Point<float>& p)
      {
        do {
          init_seeder.get_seed (p);
          if (interface.find_interface (p))
            return true;
        } while (1);
      }




      }
    }
  }
}


