/* Copyright (c) 2008-2023 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#ifndef __dwi_tractography_sift_model_h__
#define __dwi_tractography_sift_model_h__


#include "app.h"
#include "thread_queue.h"
#include "types.h"

#include "math/sphere/set/adjacency.h"

#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/streamline.h"

#include "dwi/tractography/mapping/loader.h"
#include "dwi/tractography/mapping/mapper.h"
#include "dwi/tractography/mapping/mapping.h"
#include "dwi/tractography/mapping/voxel.h"

#include "dwi/tractography/SIFT/model_base.h"
#include "dwi/tractography/SIFT/track_contribution.h"
#include "dwi/tractography/SIFT/track_index_range.h"
#include "dwi/tractography/SIFT/types.h"





namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace SIFT
      {



      // Key distinction between Model and ModelBase is that the former
      //   additionally stores the streamline-fixel intersections
      // TODO Contemplate better class naming
      class Model : public ModelBase
      {

        public:
          using ModelBase::value_type;

          Model (const std::string& fd_path);
          Model (const Model& that) = delete;

          virtual ~Model();


          // Over-rides the function defined in ModelBase
          // Need to build contributions member also
          void map_streamlines (const std::string&);


          void exclude_fixels ();

          // For debugging purposes:
          //   make sure the sum of TD in the fixels is equal to the sum of TD in the streamlines
          void check_TD();

          track_t num_tracks() const { return contributions.size(); }

          void output_non_contributing_streamlines (const std::string&) const;


        protected:

          // TODO Remove use of MinMemArray; use shrink_to_fit() instead?
          // TODO Trial use of Eigen::Sparse instead
          //   Can maybe use uint8 datatype and scale to represent intersection length
          // TODO Remove use of naked pointers
          vector<TrackContribution*> contributions;

        private:
          // Some member classes to support multi-threaded processes
          class TrackMappingWorker
          {
            public:
              TrackMappingWorker (Model& i, const default_type upsample_ratio);
              TrackMappingWorker (const TrackMappingWorker& that);
              ~TrackMappingWorker();
              bool operator() (const Tractography::Streamline<>&);
            private:
              Model& master;
              Mapping::TrackMapperBase mapper;
              std::shared_ptr<std::mutex> mutex;
              value_type TD_sum;
              vector<value_type> fixel_TDs;
              vector<track_t> fixel_counts;
          };



      };





      }
    }
  }
}


#endif
