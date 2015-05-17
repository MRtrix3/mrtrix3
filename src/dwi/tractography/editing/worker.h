/*
   Copyright 2011 Brain Research Institute, Melbourne, Australia

   Written by Robert E. Smith, 2014.

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

#ifndef __dwi_tractography_editing_worker_h__
#define __dwi_tractography_editing_worker_h__


#include <string>
#include <vector>

#include "dwi/tractography/properties.h"
#include "dwi/tractography/resample.h"
#include "dwi/tractography/streamline.h"



namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Editing {




        class Worker
        {

          public:
            Worker (Tractography::Properties& p, const size_t upsample_ratio, const size_t downsample_ratio, const bool inv, const bool end) :
              properties (p),
              upsampler (upsample_ratio),
              downsampler (downsample_ratio),
              inverse (inv),
              ends_only (end),
              thresholds (p),
              include_visited (properties.include.size(), false) { }

            Worker (const Worker& that) :
              properties (that.properties),
              upsampler (that.upsampler),
              downsampler (that.downsampler),
              inverse (that.inverse),
              ends_only (that.ends_only),
              thresholds (that.thresholds),
              include_visited (properties.include.size(), false) { }


            bool operator() (const Streamline&, Streamline&) const;


          private:
            const Tractography::Properties& properties;
            Upsampler upsampler;
            Downsampler downsampler;
            const bool inverse, ends_only;

            class Thresholds
            {
              public:
                Thresholds (Tractography::Properties&);
                Thresholds (const Thresholds&);
                bool operator() (const Streamline&) const;
              private:
                size_t max_num_points, min_num_points;
                float max_weight, min_weight;
            } thresholds;

            mutable std::vector<bool> include_visited;

        };




      }
    }
  }
}

#endif
