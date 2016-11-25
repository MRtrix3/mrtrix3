/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#ifndef __dwi_tractography_seeding_dynamic_h__
#define __dwi_tractography_seeding_dynamic_h__

#include <fstream>
#include <queue>
#include <atomic>

#include "transform.h"
#include "thread_queue.h"
#include "dwi/fmls.h"
#include "dwi/directions/set.h"
#include "dwi/tractography/ACT/tissues.h"
#include "dwi/tractography/ACT/gmwmi.h"
#include "dwi/tractography/mapping/voxel.h"
#include "dwi/tractography/seeding/base.h"
#include "dwi/tractography/SIFT/model_base.h"
#include "dwi/tractography/tracking/generated_track.h"
#include "dwi/tractography/tracking/write_kernel.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"




//#define DYNAMIC_SEED_DEBUGGING



// TD_sum is set to this at the start of execution to prevent a divide_by_zero error
#define DYNAMIC_SEED_INITIAL_TD_SUM 1e-6


// Initial seeding probability: Smaller will be slower, but allow under-reconstructed fixels
//   to be seeded more densely
#define DYNAMIC_SEED_INITIAL_PROB 1e-3


// Applicable for approach 2 with correlation term only:
// How much of the projected change in seed probability is included in seeds outside the fixel
#define DYNAMIC_SEEDING_DAMPING_FACTOR 0.5



namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {

      namespace ACT { class GMWMI_finder; }

      namespace Seeding
      {



      class Fixel_TD_seed : public SIFT::FixelBase
      { MEMALIGN(Fixel_TD_seed)

        public:
          Fixel_TD_seed (const FMLS::FOD_lobe& lobe) :
            SIFT::FixelBase (lobe),
            voxel (-1, -1, -1),
            TD (SIFT::FixelBase::TD),
            update (true),
            old_prob (DYNAMIC_SEED_INITIAL_PROB),
            applied_prob (old_prob),
            track_count_at_last_update (0),
            seed_count (0) {
              updating.clear();
            }

          Fixel_TD_seed (const Fixel_TD_seed& that) :
            SIFT::FixelBase (that),
            voxel (that.voxel),
            TD (double(that.TD)),
            update (that.update),
            old_prob (that.old_prob),
            applied_prob (that.applied_prob),
            track_count_at_last_update (that.track_count_at_last_update),
            seed_count (that.seed_count) { 
              updating.clear();
            }

          Fixel_TD_seed() :
            SIFT::FixelBase (),
            voxel (-1, -1, -1),
            TD (0.0),
            update (true),
            old_prob (DYNAMIC_SEED_INITIAL_PROB),
            applied_prob (old_prob),
            track_count_at_last_update (0),
            seed_count (0) { 
              updating.clear();
            }


          double         get_TD     ()                    const { return TD.load (std::memory_order_relaxed); }
          void           clear_TD   ()                          { TD.store (0.0, std::memory_order_acq_rel); }
          double         get_diff   (const double mu)     const { return ((TD.load (std::memory_order_relaxed) * mu) - FOD); }

          void mask() { update = false; }
          bool can_update() const { return update; }


          Fixel_TD_seed& operator+= (const double length)
          {
            // Apparently the first version may be preferable due to bugs in earlier compiler versions...
            //double old_TD, new_TD;
            //do {
            //  old_TD = TD.load (std::memory_order_relaxed);
            //  new_TD = old_TD + length;
            //} while (!TD.compare_exchange_weak (old_TD, new_TD, std::memory_order_relaxed));
            double old_TD = TD.load (std::memory_order_relaxed);
            while (!TD.compare_exchange_weak (old_TD, old_TD + length, std::memory_order_relaxed));
            return *this;
          }


          void set_voxel (const Eigen::Vector3i& i) { voxel = i; }
          const Eigen::Vector3i& get_voxel() const { return voxel; }

          float get_ratio (const double mu) const { return ((mu * TD.load (std::memory_order_relaxed)) / FOD); }


          float get_cumulative_prob (const uint64_t track_count)
          {
            while (updating.test_and_set (std::memory_order_acquire));
            float cumulative_prob = old_prob;
            if (track_count > track_count_at_last_update) {
              cumulative_prob = ((track_count_at_last_update * old_prob) + ((track_count - track_count_at_last_update) * applied_prob)) / float(track_count);
              old_prob = cumulative_prob;
              track_count_at_last_update = track_count;
            }
            return cumulative_prob;
          }

          void update_prob (const float new_prob, const bool seed_drawn)
          {
            applied_prob = new_prob;
            if (seed_drawn)
              ++seed_count;
            updating.clear (std::memory_order_release);
          }


          float get_old_prob()   const { return old_prob; }
          float get_prob()       const { return applied_prob; }
          size_t get_seed_count() const { return seed_count; }



        private:
          Eigen::Vector3i voxel;
          std::atomic<double> TD; // Protect against concurrent reads & writes, though perfect thread concurrency is not necessary
          bool update; // For small / noisy fixels, exclude the seeding probability from being updated

          // Multiple values to update - use an atomic boolean in a similar manner to a mutex, but with less overhead
          std::atomic_flag updating;
          float old_prob, applied_prob;
          size_t track_count_at_last_update;
          size_t seed_count;

      };




      class Dynamic_ACT_additions
      { MEMALIGN(Dynamic_ACT_additions)

        public:
          Dynamic_ACT_additions (const std::string& path) :
            interp_template (Image<float>::open (path)),
            gmwmi_finder (interp_template) { }

          bool check_seed (Eigen::Vector3f&);

        private:
          Interp::Linear<Image<float>> interp_template;
          ACT::GMWMI_finder gmwmi_finder;


      };





      class Dynamic : public Base, public SIFT::ModelBase<Fixel_TD_seed>
        { MEMALIGN(Dynamic)
          private:

            typedef Fixel_TD_seed Fixel;

            typedef Fixel_map<Fixel>::MapVoxel MapVoxel;
            typedef Fixel_map<Fixel>::VoxelAccessor VoxelAccessor;


        public:
        Dynamic (const std::string&, Image<float>&, const size_t, const DWI::Directions::FastLookupSet&);
        ~Dynamic();

        Dynamic (const Dynamic&) = delete;
        Dynamic& operator= (const Dynamic&) = delete;

        bool get_seed (Eigen::Vector3f&) const override;
        bool get_seed (Eigen::Vector3f&, Eigen::Vector3f&) override;

        // Although the ModelBase version of this function is OK, the Fixel_TD_seed class
        //   includes the voxel location for easier determination of seed location
        bool operator() (const FMLS::FOD_lobes&) override;

        bool operator() (const Mapping::SetDixel& i) override
        {
          if (!i.weight) // Flags that tracking should terminate
            return false;
          if (!i.empty()) {
#ifdef DYNAMIC_SEED_DEBUGGING
            const size_t updated_count = ++track_count;
            if (updated_count == target_trackcount / 2)
              output_fixel_images();
            if (updated_count >= target_trackcount)
              return false;
#else
            if (++track_count >= target_trackcount)
              return false;
#endif
          }
          return SIFT::ModelBase<Fixel_TD_seed>::operator() (i);
        }


          private:
            using Fixel_map<Fixel>::accessor;
            using Fixel_map<Fixel>::fixels;

            using SIFT::ModelBase<Fixel>::mu;
            using SIFT::ModelBase<Fixel>::proc_mask;

        // New members required for new dynamic seed probability equation
        const size_t target_trackcount;
        std::atomic<size_t> track_count;

        // Want to know statistics on dynamic seeding sampling
        std::atomic<uint64_t> attempts, seeds;


#ifdef DYNAMIC_SEED_DEBUGGING
        Tractography::Writer<float> seed_output;
        void write_seed (const Eigen::Vector3f&);
        size_t test_fixel;
        void output_fixel_images();
#endif

            Transform transform;

        std::unique_ptr<Dynamic_ACT_additions> act;

        void perform_fixel_masking();

      };





      class WriteKernelDynamic : public Tracking::WriteKernel
        { MEMALIGN(WriteKernelDynamic)
          public:
            WriteKernelDynamic (const Tracking::SharedBase& shared, const std::string& output_file, const Properties& properties) :
              Tracking::WriteKernel (shared, output_file, properties) { }
          WriteKernelDynamic (const WriteKernelDynamic&) = delete;
          WriteKernelDynamic& operator= (const WriteKernelDynamic&) = delete;
          bool operator() (const Tracking::GeneratedTrack&, Streamline<>&);
      };





      }
    }
  }
}

#endif

