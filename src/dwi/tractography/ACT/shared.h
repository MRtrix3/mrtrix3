/* Copyright (c) 2008-2023 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#ifndef __dwi_tractography_act_shared_h__
#define __dwi_tractography_act_shared_h__

#include "memory.h"
#include "dwi/tractography/ACT/act.h"
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
              bt (false),
              trunc (sgm_trunc_enum::DEFAULT)
            {
              verify_5TT_image (voxel);
              property_set.set (bt, "backtrack");
              if (property_set.find ("crop_at_gmwmi") != property_set.end())
                gmwmi_finder.reset (new GMWMI_finder (voxel));
              auto sgm_trunc_property = property_set.find ("sgm_truncation");
              if (sgm_trunc_property != property_set.end()) {
                if (sgm_trunc_property->second == "entry")
                  trunc = sgm_trunc_enum::ENTRY;
                else if (sgm_trunc_property->second == "exit")
                  trunc = sgm_trunc_enum::EXIT;
                else if (sgm_trunc_property->second == "minimum")
                  trunc = sgm_trunc_enum::MINIMUM;
                else if (sgm_trunc_property->second == "random")
                  trunc = sgm_trunc_enum::RANDOM;
                else if (sgm_trunc_property->second == "roulette")
                  trunc = sgm_trunc_enum::ROULETTE;
                else
                  throw Exception ("Invalid setting for \"sgm_truncation\" tracking property: \"" + sgm_trunc_property->second + "\"");
              }
            }


            bool backtrack() const { return bt; }
            bool crop_at_gmwmi() const { return bool (gmwmi_finder); }
            sgm_trunc_enum sgm_trunc() const { return trunc; }

            void set_sgm_trunc (const sgm_trunc_enum value) { trunc = value; }

            void crop_at_gmwmi (vector<Eigen::Vector3f>& tck) const
            {
              assert (gmwmi_finder);
              tck.back() = gmwmi_finder->find_interface (tck, true);
            }


          private:
            Image<float> voxel;
            bool bt;
            std::unique_ptr<GMWMI_finder> gmwmi_finder;
            sgm_trunc_enum trunc;


          protected:
            friend class ACT_Method_additions;

        };


      }
    }
  }
}

#endif
