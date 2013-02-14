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

// For SIFT::SH_writer
// TODO Perhaps this should be moved to FMLS.h / its own file in src/dwi/?
// Would need to template both DWI buffer and mask, have a base class without mask?
#include "dwi/tractography/SIFT/multithread.h"

#include "dwi/fmls.h"

#include "math/SH.h"



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




      Dynamic::Dynamic (const std::string& in, Image::Buffer<float>& fod_data, const Math::RNG& rng, const DWI::Directions::FastLookupSet& dirs) :
            Base (in, rng, "dynamic"),
            Mapping::FOD_TD_diff_map<FOD_TD_seed> (fod_data, dirs),
            total_samples (0),
            total_seeds   (0),
            transform (Mapping::FOD_TD_diff_map<FOD_TD_seed>::info())
#ifdef DYNAMIC_SEED_DEBUGGING
          , seed_output ("seeds.tck", Tractography::Properties())
#endif
      {
        App::Options opt = App::get_options ("act");
        if (opt.size())
          act = new Dynamic_ACT_additions (opt[0][0]);
        SIFT::FODQueueWriter<Image::Buffer<float>, Image::BufferScratch<float> > writer (fod_data, proc_mask);
        DWI::FOD_FMLS fmls (dirs, Math::SH::LforN (fod_data.dim (3)));
        fmls.set_dilate_lookup_table (true);
        Thread::run_queue_threaded_pipe (writer, DWI::SH_coefs(), fmls, DWI::FOD_lobes(), *this);

        // Have to set a volume so that Seeding::List works correctly
        for (std::vector<Lobe>::const_iterator i = lobes.begin(); i != lobes.end(); ++i)
          volume += i->get_weight();
        volume *= (fod_data.vox(0) * fod_data.vox(1) * fod_data.vox(2));

        // Prevent divide-by-zero at commencement
        Mapping::FOD_TD_diff_map<FOD_TD_seed>::TD_sum = DYNAMIC_SEED_INITIAL_TD_SUM;

      }


      Dynamic::~Dynamic()
      {

        INFO ("Dynamic seeeding required " + str (total_samples) + " samples to draw " + str (total_seeds) + " seeds");

#ifdef DYNAMIC_SEED_DEBUGGING
        const double final_mu = mu();

        // Output seeding probabilites at end of execution
        Image::Header H;
        H.info() = info();
        H.datatype() = DataType::Float32;
        Image::Buffer<float> prob_mean_data ("seed_prob_mean.mif", H), prob_sum_data ("seed_prob_sum.mif", H);
        Image::Buffer<float>::voxel_type prob_mean (prob_mean_data), prob_sum (prob_sum_data);
        VoxelAccessor v (accessor);
        Image::Loop loop;
        for (loop.start (v, prob_mean, prob_sum); loop.ok(); loop.next (v, prob_mean, prob_sum)) {
          if (v.value()) {

            float sum = 0.0;
            size_t count = 0;
            for (FOD_map<FOD_TD_seed>::ConstIterator i = begin (v); i; ++i) {
              sum += i().get_seed_prob (final_mu);
              ++count;
            }
            prob_mean.value() = sum / float(count);
            prob_sum .value() = sum;

          }
        }
#endif

      }




      bool Dynamic::get_seed (Point<float>& p, Point<float>& d)
      {

        uint64_t samples = 0;

        while (1) {

          ++samples;
          const size_t lobe_index = 1 + rng.uniform_int (lobes.size() - 1);
          const Lobe& lobe = lobes[lobe_index];

          // TODO Is rng.uniform() thread-safe?
          if (lobe.get_seed_prob (mu()) > rng.uniform()) {

            const Point<int>& v (lobe.get_voxel());
            const Point<float> vp (v[0]+rng.uniform()-0.5, v[1]+rng.uniform()-0.5, v[2]+rng.uniform()-0.5);
            p = transform.voxel2scanner (vp);

            bool good_seed = !act;
            if (!good_seed) {

              if (act->check_seed (p)) {
                // Make sure that the seed point has not left the intended voxel
                const Point<float> new_v_float (transform.scanner2voxel (p));
                const Point<int> new_v (Math::round (new_v_float[0]), Math::round (new_v_float[1]), Math::round (new_v_float[2]));
                good_seed = (new_v == v);
              }
            }
            if (good_seed) {
              d = lobe.get_dir();
#ifdef DYNAMIC_SEED_DEBUGGING
              write_seed (p);
#endif
              Thread::Mutex::Lock lock (mutex);
              total_samples += samples;
              ++total_seeds;
              return true;
            }

          }

        }

      }




      bool Dynamic::operator() (const FOD_lobes& in)
      {

        if (!Image::Nav::within_bounds (accessor, in.vox))
          return false;
        VoxelAccessor v (accessor);
        Image::Nav::set_pos (v, in.vox);
        if (v.value())
          throw Exception ("Error: Dynamic seeder has received multiple FOD segmentations for the same voxel!");
        v.value() = new MapVoxel (in, lobes.size());

        Image::BufferScratch<float>::voxel_type mask (proc_mask);
        const float weight = Image::Nav::get_value_at_pos (mask, in.vox);

        for (FOD_lobes::const_iterator i = in.begin(); i != in.end(); ++i) {
          lobes.push_back (Lobe (*i, weight, in.vox));
          FOD_sum += i->get_integral() * weight;
        }

        return true;

      }




      bool Dynamic::operator() (const Mapping::SetDixel& in)
      {
        if (in.empty())
          return true;
        float total_contribution = 0.0;
        for (Mapping::SetDixel::const_iterator i = in.begin(); i != in.end(); ++i) {
          const size_t lobe_index = Mapping::FOD_TD_diff_map<Lobe>::dix2lobe (*i);
          if (lobe_index) {
            lobes[lobe_index] += i->get_value();
            total_contribution += lobes[lobe_index].get_weight() * i->get_value();
          }
        }
        TD_sum += total_contribution;
        return true;
      }





#ifdef DYNAMIC_SEED_DEBUGGING
      void Dynamic::write_seed (const Point<float>& p)
      {
        static Thread::Mutex mutex;
        Thread::Mutex::Lock lock (mutex);
        std::vector< Point<float> > tck;
        tck.push_back (p);
        seed_output.append (tck);
      }
#endif



      }
    }
  }
}


