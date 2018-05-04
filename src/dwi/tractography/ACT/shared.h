/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
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


        class ACT_Shared_additions { MEMALIGN(ACT_Shared_additions)

          public:
            ACT_Shared_additions (const std::string& path, Properties& property_set) :
              voxel (Image<float>::open (path)),
              bt (false)
            {
              verify_5TT_image (voxel);
              property_set.set (bt, "backtrack");
              if (property_set.find ("crop_at_gmwmi") != property_set.end())
                gmwmi_finder.reset (new GMWMI_finder (voxel));
            }


            bool backtrack() const { return bt; }

            bool crop_at_gmwmi() const { return bool (gmwmi_finder); }
            void crop_at_gmwmi (vector<Eigen::Vector3f>& tck) const
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
