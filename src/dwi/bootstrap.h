/*
   Copyright 2010 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 15/10/10.

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

#ifndef __dwi_bootstrap_h__
#define __dwi_bootstrap_h__

#include "math/SH.h"
#include "dwi/gradient.h"


namespace MR {
  namespace DWI {

    template <class Set> class Bootstrap 
    {
      public:
        typedef typename Set::value_type value_type;

        Bootstrap (Set& DWI_dataset, const Math::Matrix<value_type>& DW_scheme, size_t l_max = 8) :
          DWI (DWI_dataset),
          grad (DW_scheme), 
          lmax (l_max)
      {
        if (grad.rows() < 7 || grad.columns() != 4) 
          throw Exception ("unexpected diffusion encoding matrix dimensions");

        info ("found " + str(grad.rows()) + "x" + str(grad.columns()) + " diffusion-weighted encoding");

        if (DWI.dim(3) != ssize_t (grad.rows())) 
          throw Exception ("number of studies in base image does not match that in encoding file");

        DWI::normalise_grad (grad);

        std::vector<int> bzeros, dwis;
        DWI::guess_DW_directions (dwis, bzeros, grad);

        {
          std::string msg ("found b=0 images in volumes [ ");
          for (size_t n = 0; n < bzeros.size(); n++) msg += str(bzeros[n]) + " ";
          msg += "]";
          info (msg);
        }

        info ("found " + str(dwis.size()) + " diffusion-weighted directions");

        Math::Matrix<float> dirs;
        DWI::gen_direction_matrix (dirs, grad, dwis);
        Math::SH::Transform<float> SHT (dirs, lmax);

        H.allocate (dwis.size(), dwis.size());
        Math::mult (H, SHT.mat_SH2A(), SHT.mat_A2SH());
      }

      protected:
        Set& DWI;
        const Math::Matrix<value_type>& DW_scheme;
        const Math::Matrix<value_type> H;

    };

  }
}

#endif




