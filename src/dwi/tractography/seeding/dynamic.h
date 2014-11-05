/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 2012.

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

#ifndef __dwi_tractography_seeding_dynamic_h__
#define __dwi_tractography_seeding_dynamic_h__


#include <atomic>

#include "image/transform.h"

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

#include <fstream>
#include <queue>




#define DYNAMIC_SEED_DEBUGGING



// TD_sum is set to this at the start of execution to prevent a divide_by_zero error
#define DYNAMIC_SEED_INITIAL_TD_SUM 1e-6


// Initial seeding probability: Smaller will be slower, but allow under-reconstructed fixels
//   to be seeded more densely
#define DYNAMIC_SEED_INITIAL_PROB 1e-3



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
      {

        public:
          Fixel_TD_seed (const FMLS::FOD_lobe& lobe) :
            SIFT::FixelBase (lobe),
            voxel (-1, -1, -1),
            TD (SIFT::FixelBase::TD),
            updating (ATOMIC_FLAG_INIT),
            old_prob (DYNAMIC_SEED_INITIAL_PROB),
            applied_prob (old_prob),
            track_count_at_last_update (0) { }

          Fixel_TD_seed (const Fixel_TD_seed& that) :
            SIFT::FixelBase (that),
            voxel (that.voxel),
            TD (double(that.TD)),
            updating (ATOMIC_FLAG_INIT),
            old_prob (that.old_prob),
            applied_prob (that.applied_prob),
            track_count_at_last_update (that.track_count_at_last_update) { }

          Fixel_TD_seed() :
            SIFT::FixelBase (),
            voxel (-1, -1, -1),
            TD (0.0),
            updating (ATOMIC_FLAG_INIT),
            old_prob (DYNAMIC_SEED_INITIAL_PROB),
            applied_prob (old_prob),
            track_count_at_last_update (0) { }


          double         get_TD     ()                    const { return TD.load (std::memory_order_relaxed); }
          void           clear_TD   ()                          { TD.store (0.0, std::memory_order_seq_cst); }
          double         get_diff   (const double mu)     const { return ((TD.load (std::memory_order_relaxed) * mu) - FOD); }


          Fixel_TD_seed& operator+= (const double length)
          {
            double old_TD, new_TD;
            do {
              old_TD = TD.load (std::memory_order_relaxed);
              new_TD = old_TD + length;
            } while (!TD.compare_exchange_weak (old_TD, new_TD, std::memory_order_acq_rel));
            return *this;
          }


          void set_voxel (const Point<int>& i) { voxel = i; }
          const Point<int>&   get_voxel() const { return voxel; }


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

          void update_prob (const float new_prob)
          {
            applied_prob = new_prob;
            updating.clear (std::memory_order_release);
          }


          float get_old_prob() const { return old_prob; }
          float get_prob()     const { return applied_prob; }



        private:
          Point<int> voxel;
          std::atomic<double> TD; // Protect against concurrent reads & writes, though perfect thread concurrency is not necessary

          // Multiple values to update - use an atomic boolean in a similar manner to a mutex, but with less overhead
          std::atomic_flag updating;
          float old_prob, applied_prob;
          size_t track_count_at_last_update;


      };




      class Dynamic_ACT_additions
      {

          typedef Image::Interp::Linear< Image::Buffer<float>::voxel_type > Interp;

        public:
          Dynamic_ACT_additions (const std::string& path) :
            data (path),
            interp_template (Image::Buffer<float>::voxel_type (data)),
            gmwmi_finder (data) { }

          bool check_seed (Point<float>&);

        private:
          Image::Buffer<float> data;
          const Interp interp_template;
          ACT::GMWMI_finder gmwmi_finder;


      };





      class Dynamic : public Base, public SIFT::ModelBase<Fixel_TD_seed>
      {

        typedef Fixel_TD_seed Fixel;

        typedef Fixel_map<Fixel>::MapVoxel MapVoxel;
        typedef Fixel_map<Fixel>::VoxelAccessor VoxelAccessor;


        public:
        Dynamic (const std::string&, Image::Buffer<float>&, const size_t, const Math::RNG&, const DWI::Directions::FastLookupSet&);
        ~Dynamic();

        Dynamic (const Dynamic&) = delete;
        Dynamic& operator= (const Dynamic&) = delete;

        bool get_seed (Point<float>&, Point<float>&);

        // Although the ModelBase version of this function is OK, the Fixel_TD_seed class
        //   includes the voxel location for easier determination of seed location
        bool operator() (const FMLS::FOD_lobes&);

        bool operator() (const Mapping::SetDixel& i)
        {
          if (!i.weight) // Flags that tracking should terminate
            return false;
          if (!i.empty()) {
            if (++track_count >= target_trackcount)
              return false;
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
        void write_seed (const Point<float>&);
#endif

        Image::Transform transform;

        Ptr<Dynamic_ACT_additions> act;

      };





      class WriteKernelDynamic : public Tracking::WriteKernel
      {
        public:
          WriteKernelDynamic (const Tracking::SharedBase& shared, const std::string& output_file, const DWI::Tractography::Properties& properties) :
              Tracking::WriteKernel (shared, output_file, properties) { }
          WriteKernelDynamic (const WriteKernelDynamic&) = delete;
          WriteKernelDynamic& operator= (const WriteKernelDynamic&) = delete;
          bool operator() (const Tracking::GeneratedTrack&, Tractography::Streamline<>&);
      };





      }
    }
  }
}

#endif

