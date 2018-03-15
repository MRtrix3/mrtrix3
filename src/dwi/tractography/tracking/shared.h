/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#ifndef __dwi_tractography_tracking_shared_h__
#define __dwi_tractography_tracking_shared_h__

#include <atomic>

#include "header.h"
#include "image.h"
#include "memory.h"
#include "transform.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/roi.h"
#include "dwi/tractography/ACT/shared.h"
#include "dwi/tractography/MACT/shared.h"
#include "dwi/tractography/resampling/downsampler.h"
#include "dwi/tractography/tracking/types.h"
#include "dwi/tractography/tracking/tractography.h"


// If this is enabled, images will be output in the current directory showing the density of streamline terminations due to different termination mechanisms throughout the brain
//#define DEBUG_TERMINATIONS


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Tracking
      {



        class SharedBase { MEMALIGN(SharedBase)

          public:

            SharedBase (const std::string& diff_path, Properties& property_set);

            virtual ~SharedBase();


            Image<float> source;
            Properties& properties;
            Eigen::Vector3f init_dir;
            size_t max_num_tracks, max_num_seeds, min_num_points, max_num_points;
            float max_angle, max_angle_rk4, cos_max_angle, cos_max_angle_rk4;
            float step_size, threshold, init_threshold;
            size_t max_seed_attempts;
            bool unidirectional, rk4, stop_on_all_include, implicit_max_num_seeds;
            DWI::Tractography::Resampling::Downsampler downsampler;

            // Additional members for ACT
            bool is_act() const { return bool (act_shared_additions); }
            const ACT::ACT_Shared_additions& act() const { return *act_shared_additions; }

            // Additional members for MACT
            bool is_mact() const { return bool (mact_shared_additions); }
            const MACT::MACT_Shared_additions& mact() const { return *mact_shared_additions; }


            float vox () const
            {
              return std::pow (source.spacing(0)*source.spacing(1)*source.spacing(2), float (1.0/3.0));
            }

            void set_step_size (float stepsize);

            // This gets overloaded for iFOD2, as each sample is output rather than just each step, and there are
            //   multiple samples per step
            virtual float internal_step_size() const { return step_size; }


            void add_termination (const term_t i)   const { terminations[i].fetch_add (1, std::memory_order_relaxed); }
            void add_rejection   (const reject_t i) const { rejections[i]  .fetch_add (1, std::memory_order_relaxed); }


#ifdef DEBUG_TERMINATIONS
            void add_termination (const term_t i, const Eigen::Vector3f& p) const;
#endif


          private:
            mutable std::atomic<size_t> terminations[TERMINATION_REASON_COUNT];
            mutable std::atomic<size_t> rejections  [REJECTION_REASON_COUNT];

            std::unique_ptr<ACT::ACT_Shared_additions> act_shared_additions;
            std::unique_ptr<MACT::MACT_Shared_additions> mact_shared_additions;

#ifdef DEBUG_TERMINATIONS
            Header debug_header;
            Image<uint32_t>* debug_images[TERMINATION_REASON_COUNT];
            const Transform transform;
#endif

        };



      }
    }
  }
}

#endif
