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

#ifndef __dwi_tractography_seeding_gmwmi_h__
#define __dwi_tractography_seeding_gmwmi_h__


#include "image.h"
#include "dwi/tractography/ACT/gmwmi.h"
#include "dwi/tractography/seeding/basic.h"



namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {

      namespace ACT { class GMWMI_finder; }

      namespace Seeding
      {



        class GMWMI_5TT_Wrapper
        { MEMALIGN(GMWMI_5TT_Wrapper)
          public:
            GMWMI_5TT_Wrapper (const std::string& path) :
                anat_data (Image<float>::open (path)) { }
            Image<float> anat_data;
        };


        class GMWMI : public Base, private GMWMI_5TT_Wrapper, private ACT::GMWMI_finder
        { MEMALIGN(GMWMI)

          public:
            using ACT::GMWMI_finder::Interp;

            GMWMI (const std::string&, const std::string&);

            bool get_seed (Eigen::Vector3f&) const override;


          private:
            Rejection init_seeder;
            const float perturb_max_step;

            bool perturb (Eigen::Vector3f&, Interp&) const;

        };




      }
    }
  }
}

#endif

