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

#include "dwi/tractography/seeding/dynamic.h"

#include "app.h"
#include "dwi/fmls.h"
#include "math/SH.h"
#include "dwi/tractography/rng.h"
#include "dwi/tractography/seeding/dynamic.h"

#include "sparse/fixel_metric.h"
#include "sparse/keys.h"
#include "sparse/image.h"



namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Seeding
      {




      bool Dynamic_ACT_additions::check_seed (Eigen::Vector3f& p)
      {

        // Needs to be thread-safe
        auto interp = interp_template;

        interp.scanner (p.cast<double>());
        const ACT::Tissues tissues (interp);

        if (tissues.get_csf() > tissues.get_wm() + tissues.get_gm())
          return false;

        if (tissues.get_wm() > tissues.get_gm())
          return true;

        // Detrimental to remove this in all cases tested
        return gmwmi_finder.find_interface (p, interp);

        //auto retval = gmwmi_finder.find_interface (p);
        //return retval;
      }




      Dynamic::Dynamic (const std::string& in, Image<float>& fod_data, const size_t num, const DWI::Directions::FastLookupSet& dirs) :
          Base (in, "dynamic", MAX_TRACKING_SEED_ATTEMPTS_DYNAMIC),
          SIFT::ModelBase<Fixel_TD_seed> (fod_data, dirs),
          target_trackcount (num),
          track_count (0),
          attempts (0),
          seeds (0),
#ifdef DYNAMIC_SEED_DEBUGGING
          seed_output ("seeds.tck", Tractography::Properties()),
          test_fixel (0),
#endif
          transform (fod_data)
      {
        auto opt = App::get_options ("act");
        if (opt.size())
          act.reset (new Dynamic_ACT_additions (opt[0][0]));

        perform_FOD_segmentation (fod_data);

        // Have to set a volume so that Seeding::List works correctly
        for (const auto& i : fixels)
          volume += i.get_weight();
        volume *= fod_data.spacing(0) * fod_data.spacing(1) * fod_data.spacing(2);

        // Prevent divide-by-zero at commencement
        SIFT::ModelBase<Fixel_TD_seed>::TD_sum = DYNAMIC_SEED_INITIAL_TD_SUM;

        // For small / unreliable fixels, don't modify the seeding probability during execution
        perform_fixel_masking();

#ifdef DYNAMIC_SEED_DEBUGGING
        // Pick a good fixel to use for testing / debugging
        do {
          test_fixel = 1 + Base::rng.uniform_int (fixels.size()-1);
        } while (fixels[test_fixel].get_weight() < 1.0 || fixels[test_fixel].get_FOD() < 0.5);
#endif
      }


      Dynamic::~Dynamic()
      {

        INFO ("Dynamic seeeding required " + str (attempts) + " samples to draw " + str (seeds) + " seeds");

#ifdef DYNAMIC_SEED_DEBUGGING

        // Update all cumulative probabilities before output
        for (size_t fixel_index = 1; fixel_index != fixels.size(); ++fixel_index) {
          Fixel& fixel = fixels[fixel_index];
          const float ratio = fixel.get_ratio (mu());
          const bool force_seed = !fixel.get_TD();
          const size_t current_trackcount = track_count.load (std::memory_order_relaxed);
          const float cumulative_prob = fixel.get_cumulative_prob (current_trackcount);
          float seed_prob = cumulative_prob;
          if (!force_seed) {
            const uint64_t total_seeds = seeds.load (std::memory_order_relaxed);
            const uint64_t fixel_seeds = fixel.get_seed_count();
            seed_prob = cumulative_prob * (current_trackcount / float(target_trackcount - current_trackcount)) * (
                        ((total_seeds / float(fixel_seeds)) * (target_trackcount / float(current_trackcount)) * ((1.0f / ratio) - ((total_seeds - fixel_seeds) / float(total_seeds))))
                        - 1.0f);
            seed_prob = std::min (1.0f, seed_prob);
            seed_prob = std::max (0.0f, seed_prob);
          }
          fixel.update_prob (seed_prob, false);
        }

        // Output seeding probabilites at end of execution
        // Also output reconstruction ratios at end of execution
        Image::Header H;
        H.info() = info();
        H.datatype() = DataType::UInt64;
        H.datatype().set_byte_order_native();
        H[Image::Sparse::name_key] = str(typeid(Image::Sparse::FixelMetric).name());
        H[Image::Sparse::size_key] = str(sizeof(Image::Sparse::FixelMetric));
        Image::BufferSparse<Image::Sparse::FixelMetric> buffer_probs ("final_seed_probs.msf", H), buffer_logprobs ("final_seed_logprobs.msf", H), buffer_ratios ("final_fixel_ratios.msf", H);
        auto out_probs = buffer_probs.voxel(), out_logprobs = buffer_logprobs.voxel(), out_ratios = buffer_ratios.voxel();
        VoxelAccessor v (accessor);
        Image::Loop loop;
        for (loop.start (v, out_probs, out_logprobs, out_ratios); loop.ok(); loop.next (v, out_probs, out_logprobs, out_ratios)) {
          if (v.value()) {
            out_probs   .value().set_size ((*v.value()).num_fixels());
            out_logprobs.value().set_size ((*v.value()).num_fixels());
            out_ratios  .value().set_size ((*v.value()).num_fixels());
            size_t index = 0;
            for (Fixel_map<Fixel_TD_seed>::ConstIterator i = begin (v); i; ++i, ++index) {
              Image::Sparse::FixelMetric fixel (i().get_dir(), i().get_FOD(), i().get_old_prob());
              out_probs.value()[index] = fixel;
              fixel.value = log10 (fixel.value);
              out_logprobs.value()[index] = fixel;
              fixel.value = mu() * i().get_TD() / i().get_FOD();
              out_ratios.value()[index] = fixel;
            }
          }
        }

#endif

      }



      bool Dynamic::get_seed (Eigen::Vector3f&) const { return false; }

      bool Dynamic::get_seed (Eigen::Vector3f& p, Eigen::Vector3f& d) 
      {

        uint64_t this_attempts = 0;
        std::uniform_int_distribution<size_t> uniform_int (0, fixels.size()-2);
        std::uniform_real_distribution<float> uniform_float (0.0f, 1.0f);

        while (1) {

          ++this_attempts;
          const size_t fixel_index = 1 + uniform_int (*rng);
          Fixel& fixel = fixels[fixel_index];
          float seed_prob;
          if (fixel.can_update()) {

            // Derive the new seed probability
            // TODO Functionalise this?
            const float ratio = fixel.get_ratio (mu());
            const bool force_seed = !fixel.get_TD();
            const size_t current_trackcount = track_count.load (std::memory_order_relaxed);
            const float cumulative_prob = fixel.get_cumulative_prob (current_trackcount);
            seed_prob = cumulative_prob;
            if (!force_seed) {

              // Target track count is double the current track count, until this exceeds the actual target number
              // - try to modify the probabilities faster at earlier stages
              const size_t Szero = std::min (target_trackcount, 2 * current_trackcount);
              seed_prob = (ratio < 1.0) ?
                  (cumulative_prob * (Szero - (current_trackcount * ratio)) / (ratio * (Szero - current_trackcount))) :
                  0.0;

#ifdef DYNAMIC_SEED_DEBUGGING
              //if (fixel_index == test_fixel)
              //  std::cerr << "Ratio " << ratio << ", tracks " << current_trackcount << "/" << target_trackcount << ", seeds " << fixel_seeds << "/" << total_seeds << ", prob " << cumulative_prob << " -> " << seed_prob << "\n";
#endif

              // These can occur fairly regularly, depending on the exact derivation
              seed_prob = std::min (1.0f, seed_prob);
              seed_prob = std::max (0.0f, seed_prob);

            }

          } else {
            seed_prob = fixel.get_old_prob();
          }

          if (seed_prob > uniform_float (*rng)) {

            const Eigen::Vector3i& v (fixel.get_voxel());
            const Eigen::Vector3f vp (v[0]+uniform_float(*rng)-0.5, v[1]+uniform_float(*rng)-0.5, v[2]+uniform_float(*rng)-0.5);
            p = transform.voxel2scanner.cast<float>() * vp;

            bool good_seed = !act;
            if (!good_seed) {

              if (act->check_seed (p)) {
                // Make sure that the seed point has not left the intended voxel
                const Eigen::Vector3f new_v_float (transform.scanner2voxel.cast<float>() * p);
                const Eigen::Vector3i new_v (std::round (new_v_float[0]), std::round (new_v_float[1]), std::round (new_v_float[2]));
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
              fixel.update_prob (seed_prob, true);
              return true;
            }

          }

          fixel.update_prob (seed_prob, false);

        }
        return false;

      }




      bool Dynamic::operator() (const FMLS::FOD_lobes& in)
      {
        if (!SIFT::ModelBase<Fixel_TD_seed>::operator() (in))
          return false;
        VoxelAccessor v (accessor());
        assign_pos_of (in.vox).to (v);
        if (v.value()) {
          for (DWI::Fixel_map<Fixel>::Iterator i = begin (v); i; ++i)
            i().set_voxel (in.vox);
        }
        return true;
      }




#ifdef DYNAMIC_SEED_DEBUGGING
      void Dynamic::write_seed (const Eigen::Vector3f& p)
      {
        static std::mutex mutex;
        std::lock_guard<std::mutex> lock (mutex);
        vector< Eigen::Vector3f > tck;
        tck.push_back (p);
        seed_output (tck);
      }

      void Dynamic::output_fixel_images()
      {

        // Output seeding probabilites at halfway point
        Image::Header H;
        H.info() = info();
        H.datatype() = DataType::UInt64;
        H.datatype().set_byte_order_native();
        H[Image::Sparse::name_key] = str(typeid(Image::Sparse::FixelMetric).name());
        H[Image::Sparse::size_key] = str(sizeof(Image::Sparse::FixelMetric));
        Image::BufferSparse<Image::Sparse::FixelMetric> buffer_probs ("mid_seed_probs.msf", H), buffer_logprobs ("mid_seed_logprobs.msf", H), buffer_ratios ("mid_fixel_ratios.msf", H), buffer_FDs ("mid_FDs.msf", H), buffer_TDs ("mid_TDs.msf", H);
        auto out_probs = buffer_probs.voxel(), out_logprobs = buffer_logprobs.voxel(), out_ratios = buffer_ratios.voxel(), out_FDs = buffer_FDs.voxel(), out_TDs = buffer_TDs.voxel();
        VoxelAccessor v (accessor);
        Image::Loop loop;
        for (auto loopiter = loop (v, out_probs, out_logprobs, out_ratios, out_FDs, out_TDs); loopiter; ++loopiter) {
          if (v.value()) {
            out_probs   .value().set_size ((*v.value()).num_fixels());
            out_logprobs.value().set_size ((*v.value()).num_fixels());
            out_ratios  .value().set_size ((*v.value()).num_fixels());
            out_FDs     .value().set_size ((*v.value()).num_fixels());
            out_TDs     .value().set_size ((*v.value()).num_fixels());
            size_t index = 0;
            for (Fixel_map<Fixel_TD_seed>::ConstIterator i = begin (v); i; ++i, ++index) {
              Image::Sparse::FixelMetric fixel (i().get_dir(), i().get_FOD(), i().get_old_prob());
              out_probs.value()[index] = fixel;
              fixel.value = log10 (fixel.value);
              out_logprobs.value()[index] = fixel;
              fixel.value = mu() * i().get_TD() / i().get_FOD();
              out_ratios.value()[index] = fixel;
              fixel.value = i().get_FOD();
              out_FDs.value()[index] = fixel;
              fixel.value = i().get_TD() * mu();
              out_TDs.value()[index] = fixel;
            }
          }
        }

      }

#endif





      void Dynamic::perform_fixel_masking()
      {
        // IDEA Rather than a hard masking, could this be instead used to 'damp' how much the seeding
        //   probabilities can be perturbed? (This could perhaps be done by scaling R if less than zero)
        // IDEA Should the initial seeding probabilities be modulated according to the number of fixels
        //   in each voxel? (i.e. equal probability per voxel = WM seeding)
        // Although this may make it more homogeneous, it may actually be detrimental due to the
        //   biases in our particular tracking
        for (size_t i = 1; i != fixels.size(); ++i) {
          // Unfortunately this is going to be model-dependent...
          if (fixels[i].get_FOD() * fixels[i].get_weight() < 0.1)
            fixels[i].mask();
        }
#ifdef DYNAMIC_SEED_DEBUGGING
        Image::Header H;
        H.info() = info();
        H.datatype() = DataType::UInt64;
        H.datatype().set_byte_order_native();
        H[Image::Sparse::name_key] = str(typeid(Image::Sparse::FixelMetric).name());
        H[Image::Sparse::size_key] = str(sizeof(Image::Sparse::FixelMetric));
        Image::BufferSparse<Image::Sparse::FixelMetric> buffer ("fixel_mask.msf", H);
        auto out = buffer.voxel();
        VoxelAccessor v (accessor);
        Image::Loop loop;
        for (loop.start (v, out); loop.ok(); loop.next (v, out)) {
          if (v.value()) {
            out.value().set_size ((*v.value()).num_fixels());
            size_t index = 0;
            for (Fixel_map<Fixel_TD_seed>::ConstIterator f = begin(v); f; ++f, ++index) {
              Image::Sparse::FixelMetric fixel (f().get_dir(), f().get_FOD(), (f().can_update() ? 1.0f : 0.0f));
              out.value()[index] = fixel;
            }
          }
        }
#endif
      }





        bool WriteKernelDynamic::operator() (const Tracking::GeneratedTrack& in, Tractography::Streamline<>& out)
        {
          out.index = writer.count;
          out.weight = 1.0;
          if (!WriteKernel::operator() (in)) {
            out.clear();
            // Flag to indicate that tracking has completed, and threads should therefore terminate
            out.weight = 0.0;
            // Actually need to pass this down the queue so that the seeder thread receives it and knows to terminate
            return true;
          }
          out = in;
          return out.size(); // New pipe functor interpretation: Don't bother sending empty tracks
        }






      }
    }
  }
}


