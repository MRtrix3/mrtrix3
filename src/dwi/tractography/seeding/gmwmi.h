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

#ifndef __dwi_tractography_seeding_gmwmi_h__
#define __dwi_tractography_seeding_gmwmi_h__


#include "dwi/tractography/ACT/gmwmi.h"

#include "dwi/tractography/seeding/basic.h"


#define GMWMI_SEED_ATTEMPTS 10000




namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {

      namespace ACT { class GMWMI_finder; }

      namespace Seeding
      {




        class GMWMI : public Base
        {

          public:
            GMWMI (const std::string&, const Math::RNG&, const std::string&);

            bool get_seed (Point<float>&);


          private:
            SeedMask init_seeder;
            Image::Buffer<float> anat_data;
            ACT::GMWMI_finder interface;

        };




      }
    }
  }
}

#endif

