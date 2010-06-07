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

#include "dwi/tractography/tracker/dt_stream.h"
#include "dwi/gradient.h"

namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Tracker {

        DTStream::DTStream (Image::Object& source_image, Properties& properties, const Math::Matrix<float>& inverse_bmat) : 
          Base (source_image, properties), 
          binv (inverse_bmat),
          V(3,3), 
          D(3,3)
        {
          float min_curv = 2.0; 

          properties["method"] = "DT_STREAM";
          if (props["min_curv"].empty()) props["min_curv"] = str (min_curv); else min_curv = to<float> (props["min_curv"]);
          if (props["max_num_tracks"].empty()) props["max_num_tracks"] = "100";

          if (binv.rows() != 7 || binv.columns() < 7) 
            throw Exception ("unexpected diffusion b-matrix dimensions");

          if (source.dim(3) != (int) binv.columns()) 
            throw Exception ("number of studies in base image does not match that in encoding file");


          min_dp = cos (curv2angle (step_size, min_curv));

          eigen_values = gsl_vector_alloc (3);
          eigv_work = gsl_eigen_symmv_alloc (3);
        }





        bool DTStream::init_direction (const Point& seed_dir)
        {
          float fa = get_EV (pos);
          if (fa < init_threshold) return (true);

          if (isnan (seed_dir[0])) {
            if (rng.uniform() < 0.5) {
              dir[0] = -dir[0];
              dir[1] = -dir[1];
              dir[2] = -dir[2];
            }
          }
          else {
            if (seed_dir.dot (dir) < 0.0) {
              dir[0] = -dir[0];
              dir[1] = -dir[1];
              dir[2] = -dir[2];
            }
          }

          return (false);
        }




        bool DTStream::next_point ()
        {
          Point prev_dir (dir);
          float fa = get_EV (pos);
          if (fa < threshold) return (true);
          fa = dir.dot (prev_dir);
          if (fabs (fa) < min_dp) return (true);
          if (fa < 0.0) {
            dir[0] = -dir[0];
            dir[1] = -dir[1];
            dir[2] = -dir[2];
          }
          inc_pos ();
          return (false);
        }

      }
    }
  }
}


