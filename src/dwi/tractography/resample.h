/*
   Copyright 2011 Brain Research Institute, Melbourne, Australia

   Written by Robert E. Smith, 2011.

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

#ifndef __dwi_tractography_resample_h__
#define __dwi_tractography_resample_h__


#include <vector>

#include "math/hermite.h"
#include "dwi/tractography/tracking/generated_track.h"


namespace MR {
  namespace DWI {
    namespace Tractography {


        class Upsampler
        {

          public:
            Upsampler () :
              data (4, 3) { }

            Upsampler (const size_t os_ratio) :
              data (4, 3) {
                set_ratio (os_ratio);
              }

            Upsampler (const Upsampler& that) :
              M    (that.M),
              temp (M.rows(), 3),
              data (4, 3) { }

            ~Upsampler() { }


            void set_ratio (const size_t);
            bool operator() (std::vector<Eigen::Vector3f>&) const;

            size_t get_ratio() const { return (M.rows() ? (M.rows() + 1) : 1); }
            bool   valid ()    const { return (M.rows()); }


          private:
            Eigen::MatrixXf M;
            mutable Eigen::MatrixXf temp, data;

            bool interp_prepare (std::vector<Eigen::Vector3f>&) const;
            void increment (const Eigen::Vector3f&) const;

        };











      class Downsampler
      {

        public:
          Downsampler () : ratio (1) { }
          Downsampler (const size_t downsample_ratio) : ratio (downsample_ratio) { }

          bool operator() (Tracking::GeneratedTrack&) const;
          bool operator() (std::vector<Eigen::Vector3f>&) const;

          bool valid() const { return (ratio > 1); }
          size_t get_ratio() const { return ratio; }
          void set_ratio (const size_t i) { ratio = i; }

        private:
          size_t ratio;

      };





    }
  }
}

#endif



