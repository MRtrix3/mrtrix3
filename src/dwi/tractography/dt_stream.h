/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

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


    18-12-2008 J-Donald Tournier <d.tournier@brain.org.au>
    * modify eigenvector computation to allow thread-safe operation

*/

#ifndef __dwi_tractography_tracker_dt_stream_h__
#define __dwi_tractography_tracker_dt_stream_h__

#include <gsl/gsl_eigen.h>
#include <gsl/gsl_sort_vector.h>

#include "dwi/tractography/tracker/base.h"
#include "dwi/tensor.h"

namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Tracker {

        class DTStream : public Base {
          public:
            DTStream (Image::Object& source_image, Properties& properties, const Math::Matrix<float>& inverse_bmat);
            ~DTStream () { gsl_eigen_symmv_free (eigv_work); gsl_vector_free (eigen_values); }

          protected:
            virtual bool  init_direction (const Point& seed_dir);
            virtual bool  next_point ();

            const Math::Matrix<double>& binv;
            Math::Matrix<double>  V, D;
            float         min_dp;

            gsl_vector*                 eigen_values;
            gsl_eigen_symmv_workspace*  eigv_work;

            float         get_EV (const Point& p);
        };








        inline float DTStream::get_EV (const Point& p)
        {
          float values[source.dim(3)];
          if (get_source_data (p, values)) return (-1.0);

          for (int n = 0; n < source.dim(3); n++) 
            values[n] = values[n] > 0.0 ? -log (values[n]) : 1e-12;

          float dt[6];
          for (int n = 0; n < 6; n++) {
            dt[n] = 0.0;
            for (int i = 0; i < source.dim(3); i++)
              dt[n] += (float) (binv(n, i) * values[i]);
          }

          D(0,0) = dt[0];
          D(1,1) = dt[1];
          D(2,2) = dt[2];
          D(0,1) = D(1,0) = dt[3];
          D(0,2) = D(2,0) = dt[4];
          D(1,2) = D(2,1) = dt[5]; 

          gsl_eigen_symmv (&D, eigen_values, &V, eigv_work);
          gsl_eigen_symmv_sort (eigen_values, &V, GSL_EIGEN_SORT_VAL_ASC);

          dir[0] = V(0,2);
          dir[1] = V(1,2);
          dir[2] = V(2,2);

          return (tensor2FA (dt));
        }


      }
    }
  }
}

#endif

