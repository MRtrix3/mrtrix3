/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 25/10/09.

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

#ifndef __dwi_tractography_FACT_h__
#define __dwi_tractography_FACT_h__

#include "point.h"
#include "math/eigen.h"
#include "dwi/gradient.h"
#include "dwi/tensor.h"
#include "dwi/tractography/method.h"
#include "dwi/tractography/shared.h"


namespace MR {
  namespace DWI {
    namespace Tractography {

      class FACT : public MethodBase {
        public:
          class Shared : public SharedBase {
            public:
              Shared (Image::Header& source, DWI::Tractography::Properties& property_set) :
                SharedBase (source, property_set) {

                  set_step_size (0.1);
                  info ("minimum radius of curvature = " + str(step_size / ( 2.0 * sin (max_angle / 2.0))) + " mm");

                  properties["method"] = "FACT";

                  Math::Matrix<value_type> grad;
                  if (properties.find ("DW_scheme") != properties.end())
                    grad.load (properties["DW_scheme"]);
                  else
                    grad = source.DW_scheme();

                  if (grad.columns() != 4) 
                    throw Exception ("unexpected number of columns in gradient encoding (expected 4 columns)");
                  if (grad.rows() < 7) 
                    throw Exception ("too few rows in gradient encoding (need at least 7)");

                  normalise_grad (grad);

                  grad2bmatrix (bmat, grad);
                  Math::pinv (binv, bmat);

                  cos_max_angle = Math::cos (max_angle);
                }

              Math::Matrix<value_type> bmat, binv;
              value_type cos_max_angle;
          };






          FACT (const Shared& shared) : 
            MethodBase (shared), 
            S (shared),
            eig (3),
            M (3,3),
            V (3,3),
            ev (3) {
            } 




          bool init () 
          { 
            if (!get_data ()) 
              return false;

            return do_init();
          }   



          bool next () 
          {
            if (!get_data ())
              return false;

            return do_next();
          }

        protected:
          const Shared& S;
          Math::Eigen::SymmV<double> eig;
          Math::Matrix<double> M, V;
          Math::Vector<double> ev;

          void get_EV () 
          {
            M(0,0) = values[0];
            M(1,1) = values[1];
            M(2,2) = values[2];
            M(0,1) = M(1,0) = values[3];
            M(0,2) = M(2,0) = values[4];
            M(1,2) = M(2,1) = values[5];

            eig (ev, M, V);
            Math::Eigen::sort (ev, V);

            dir[0] = V(0,2);
            dir[1] = V(1,2);
            dir[2] = V(2,2);
          }


          bool do_init ()
          {
            dwi2tensor (S.binv, values);

            if (tensor2FA (values) < S.init_threshold)
              return false;

            get_EV();

            return true;
          }


          bool do_next () 
          {
            dwi2tensor (S.binv, values);

            if (tensor2FA (values) < S.threshold)
              return false;

            Point<value_type> prev_dir = dir;

            get_EV();

            value_type dot = prev_dir.dot (dir);
            if (Math::abs (dot) < S.cos_max_angle) 
              return false;

            if (dot < 0.0) 
              dir = -dir;

            pos += dir * S.step_size;

            return true;
          }

      };

    }
  }
}

#endif


