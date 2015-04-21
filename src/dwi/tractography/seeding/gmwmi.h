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


#include "image/buffer.h"

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



        class GMWMI_5TT_Wrapper
        {
          public:
            GMWMI_5TT_Wrapper (const std::string& path) :
                anat_data (path) { }
            Image::Buffer<float> anat_data;
        };


        class GMWMI : public Base, private GMWMI_5TT_Wrapper, private ACT::GMWMI_finder
        {

          public:
            using ACT::GMWMI_finder::Interp;

            GMWMI (const std::string&, const std::string&);

            bool get_seed (Point<float>&);


          private:
            Rejection init_seeder;
            const float perturb_max_step;

            bool perturb (Point<float>&, Interp&);

        };




      }
    }
  }
}

#endif

