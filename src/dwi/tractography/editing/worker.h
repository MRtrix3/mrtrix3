/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


#ifndef __dwi_tractography_editing_worker_h__
#define __dwi_tractography_editing_worker_h__


#include <string>
#include <vector>

#include "point.h"

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
            Worker (Tractography::Properties& p, const size_t upsample_ratio, const size_t downsample_ratio) :
              properties (p),
              upsampler (upsample_ratio),
              downsampler (downsample_ratio),
              thresholds (p),
              include_visited (properties.include.size(), false) { }

            Worker (const Worker& that) :
              properties (that.properties),
              upsampler (that.upsampler),
              downsampler (that.downsampler),
              thresholds (that.thresholds),
              include_visited (properties.include.size(), false) { }


            bool operator() (const Tractography::Streamline<>&, Tractography::Streamline<>&);


          private:
            const Tractography::Properties& properties;
            Upsampler<> upsampler;
            Downsampler downsampler;

            class Thresholds
            {
              public:
                Thresholds (Tractography::Properties&);
                Thresholds (const Thresholds&);
                bool operator() (const Tractography::Streamline<>&) const;
              private:
                size_t max_num_points, min_num_points;
                float max_weight, min_weight;
            } thresholds;

            std::vector<bool> include_visited;

        };




      }
    }
  }
}

#endif
