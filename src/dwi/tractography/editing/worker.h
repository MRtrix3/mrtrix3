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
