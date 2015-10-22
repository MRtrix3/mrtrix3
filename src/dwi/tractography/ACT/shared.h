/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

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

#ifndef __dwi_tractography_act_shared_h__
#define __dwi_tractography_act_shared_h__

#include "memory.h"
#include "dwi/tractography/ACT/gmwmi.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace ACT
      {


        class ACT_Shared_additions {

          public:
            ACT_Shared_additions (const std::string& path, Properties& property_set) :
              voxel (Image<float>::open (path)),
              bt (false)
            {
              verify_5TT_image (voxel.header());
              property_set.set (bt, "backtrack");
              if (property_set.find ("crop_at_gmwmi") != property_set.end())
                gmwmi_finder.reset (new GMWMI_finder (voxel));
            }


            bool backtrack() const { return bt; }

            bool crop_at_gmwmi() const { return bool (gmwmi_finder); }
            void crop_at_gmwmi (std::vector<Eigen::Vector3f>& tck) const
            {
              assert (gmwmi_finder);
              tck.back() = gmwmi_finder->find_interface (tck, true);
            }


          private:
            Image<float> voxel;
            bool bt;

            std::unique_ptr<GMWMI_finder> gmwmi_finder;


          protected:
            friend class ACT_Method_additions;

        };


      }
    }
  }
}

#endif
