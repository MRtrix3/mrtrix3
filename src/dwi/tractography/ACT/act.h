/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 02/02/12.

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

#ifndef __dwi_tractography_act_act_h__
#define __dwi_tractography_act_act_h__

#include "app.h"
#include "point.h"

#include "image/header.h"


// Actually think it's preferable to not use these
#define ACT_WM_INT_REQ 0.0
#define ACT_WM_ABS_REQ 0.0

#define GMWMI_ACCURACY 0.01 // Absolute value of tissue proportion difference

// Number of times a backtrack attempt will be made from a certain maximal track length before the length of truncation is increased
#define ACT_BACKTRACK_ATTEMPTS 3


namespace MR
{

  namespace App { class OptionGroup; }

  namespace DWI
  {

    namespace Tractography
    {

      class Properties;

      namespace ACT
      {

        extern const App::OptionGroup ACTOption;

        void load_act_properties (Properties& properties);


        void verify_5TT_image (const Image::Header&);


      }
    }
  }
}

#endif

