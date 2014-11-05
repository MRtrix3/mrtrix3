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


#include "dwi/tractography/seeding/dynamic.h"

#include "app.h"

#include "image/nav.h"

#include "dwi/fmls.h"

#include "math/SH.h"

#include "image/buffer_sparse.h"
#include "image/sparse/fixel_metric.h"
#include "image/sparse/keys.h"
#include "image/sparse/voxel.h"



namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Seeding
      {




      bool Dynamic_ACT_additions::check_seed (Point<float>& p)
      {

        // Needs to be thread-safe
        Interp interp (interp_template);

        interp.scanner (p);
        const ACT::Tissues tissues (interp);

        if (tissues.get_csf() > tissues.get_wm() + tissues.get_gm())
          return false;

        if (tissues.get_wm() > tissues.get_gm())
          return true;

        return gmwmi_finder.find_interface (p);

      }




      Dynamic::Dynamic (const std::string& in, Image::Buffer<float>& fod_data, const size_t num, const Math::RNG& rng, const DWI::Directions::FastLookupSet& dirs) :
          Base (in, rng, "dynamic", MAX_TRACKING_SEED_ATTEMPTS_DYNAMIC),
          SIFT::ModelBase<Fixel_TD_seed> (fod_data, dirs),
          target_trackcount (num),
          track_count (0),
          attempts (0),
          seeds (0),
#ifdef DYNAMIC_SEED_DEBUGGING
          seed_output ("seeds.tck", Tractography::Properties()),
#endif
          transform (SIFT::ModelBase<Fixel_TD_seed>::info())
      {
        App::Options opt = App::get_options ("act");
        if (opt.size())
          act = new Dynamic_ACT_additions (opt[0][0]);

        perform_FOD_segmentation (fod_data);

        // Have to set a volume so that Seeding::List works correctly
        for (std::vector<Fixel>::const_iterator i = fixels.begin(); i != fixels.end(); ++i)
          volume += i->get_weight();
        volume *= (fod_data.vox(0) * fod_data.vox(1) * fod_data.vox(2));

        // Prevent divide-by-zero at commencement
        SIFT::ModelBase<Fixel_TD_seed>::TD_sum = DYNAMIC_SEED_INITIAL_TD_SUM;
      }


      Dynamic::~Dynamic()
      {

        INFO ("Dynamic seeeding required " + str (attempts) + " samples to draw " + str (seeds) + " seeds");

#ifdef DYNAMIC_SEED_DEBUGGING
        // Output seeding probabilites at end of execution
        // Also output reconstruction ratios at end of execution
        Image::Header H;
        H.info() = info();
        H.datatype() = DataType::UInt64;
        H.datatype().set_byte_order_native();
        H[Image::Sparse::name_key] = str(typeid(Image::Sparse::FixelMetric).name());
        H[Image::Sparse::size_key] = str(sizeof(Image::Sparse::FixelMetric));
        Image::BufferSparse<Image::Sparse::FixelMetric> buffer_probs ("seed_probs.msf", H), buffer_ratios ("fixel_ratios.msf", H);
        auto out_probs = buffer_probs.voxel(), out_ratios = buffer_ratios.voxel();
        VoxelAccessor v (accessor);
        Image::Loop loop;
        for (loop.start (v, out_probs, out_ratios); loop.ok(); loop.next (v, out_probs, out_ratios)) {
          if (v.value()) {
            out_probs .value().set_size ((*v.value()).num_fixels());
            out_ratios.value().set_size ((*v.value()).num_fixels());
            size_t index = 0;
            for (Fixel_map<Fixel_TD_seed>::ConstIterator i = begin (v); i; ++i, ++index) {
              Image::Sparse::FixelMetric fixel (i().get_dir(), i().get_FOD(), i().get_old_prob());
              out_probs.value()[index] = fixel;
              fixel.value = mu() * i().get_TD() / i().get_FOD();
              out_ratios.value()[index] = fixel;
            }
          }
        }
#endif

      }




      bool Dynamic::get_seed (Point<float>& p, Point<float>& d)
      {

        uint64_t this_attempts = 0;

        while (1) {

          ++this_attempts;
          const size_t fixel_index = 1 + rng.uniform_int (fixels.size() - 1);
          Fixel& fixel = fixels[fixel_index];

          // Derive the new seed probability
          const float ratio = fixel.get_ratio (mu());
          const bool force_seed = !fixel.get_TD();
          const size_t current_trackcount = track_count.load (std::memory_order_relaxed);
          const float cumulative_prob = fixel.get_cumulative_prob (current_trackcount);
          float seed_prob = cumulative_prob;
          if (!force_seed) {
            // TODO See if allowing seeding in over-defined fixels helps
            //seed_prob = std::min (1.0f, cumulative_prob * (target_trackcount - (current_trackcount * ratio)) / (ratio * (target_trackcount - current_trackcount)));
            seed_prob = (ratio < 1.0) ?
                        (cumulative_prob * (target_trackcount - (current_trackcount * ratio)) / (ratio * (target_trackcount - current_trackcount))) :
                        0.0;
          }
          fixel.update_prob (seed_prob);

          if (seed_prob > rng.uniform()) {

            const Point<int>& v (fixel.get_voxel());
            const Point<float> vp (v[0]+rng.uniform()-0.5, v[1]+rng.uniform()-0.5, v[2]+rng.uniform()-0.5);
            p = transform.voxel2scanner (vp);

            bool good_seed = !act;
            if (!good_seed) {

              if (act->check_seed (p)) {
                // Make sure that the seed point has not left the intended voxel
                const Point<float> new_v_float (transform.scanner2voxel (p));
                const Point<int> new_v (std::round (new_v_float[0]), std::round (new_v_float[1]), std::round (new_v_float[2]));
                good_seed = (new_v == v);
              }
            }
            if (good_seed) {
              d = fixel.get_dir();
#ifdef DYNAMIC_SEED_DEBUGGING
              write_seed (p);
#endif
              attempts.fetch_add (this_attempts, std::memory_order_relaxed);
              seeds.fetch_add (1, std::memory_order_relaxed);
              return true;
            }

          }

        }
        return false;

      }




      bool Dynamic::operator() (const FMLS::FOD_lobes& in)
      {
        if (!SIFT::ModelBase<Fixel_TD_seed>::operator() (in))
          return false;
        VoxelAccessor v (accessor);
        Image::Nav::set_pos (v, in.vox);
        if (v.value()) {
          for (DWI::Fixel_map<Fixel>::Iterator i = begin (v); i; ++i)
            i().set_voxel (in.vox);
        }
        return true;
      }




#ifdef DYNAMIC_SEED_DEBUGGING
      void Dynamic::write_seed (const Point<float>& p)
      {
        static std::mutex mutex;
        std::lock_guard<std::mutex> lock (mutex);
        std::vector< Point<float> > tck;
        tck.push_back (p);
        seed_output (tck);
      }
#endif






        bool WriteKernelDynamic::operator() (const Tracking::GeneratedTrack& in, Tractography::Streamline<>& out)
        {
          out.index = writer.count;
          out.weight = 1.0;
          // With new pipe functors, should be possible to avoid sending empty tracks down the queue
          if (in.empty()) {
            out.clear();
            return false;
          }
          if (!WriteKernel::operator() (in)) {
            out.clear();
            // Flag to indicate that tracking has completed, and threads should therefore terminate
            out.weight = 0.0;
            // Actually need to pass this down the queue so that the seeder thread receives it and knows to terminate
            return true;
          }
          out = in;
          return true;
        }






      }
    }
  }
}


