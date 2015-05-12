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


#include "app.h"
#include "image/nav.h"
#include "dwi/fmls.h"
#include "math/SH.h"
#include "dwi/tractography/seeding/dynamic.h"


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




      Dynamic::Dynamic (const std::string& in, Image::Buffer<float>& fod_data, const DWI::Directions::FastLookupSet& dirs) :
          Base (in, "dynamic", MAX_TRACKING_SEED_ATTEMPTS_DYNAMIC),
          SIFT::ModelBase<Fixel_TD_seed> (fod_data, dirs),
          total_samples (0),
          total_seeds   (0),
          transform (SIFT::ModelBase<Fixel_TD_seed>::info())
#ifdef DYNAMIC_SEED_DEBUGGING
        , seed_output ("seeds.tck", Tractography::Properties())
#endif
      {
        App::Options opt = App::get_options ("act");
        if (opt.size())
          act.reset (new Dynamic_ACT_additions (opt[0][0]));

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

        INFO ("Dynamic seeeding required " + str (total_samples) + " samples to draw " + str (total_seeds) + " seeds");

#ifdef DYNAMIC_SEED_DEBUGGING
        const double final_mu = mu();

        // Output seeding probabilites at end of execution
        Image::Header H;
        H.info() = info();
        H.datatype() = DataType::Float32;
        Image::Buffer<float> prob_mean_data ("seed_prob_mean.mif", H), prob_sum_data ("seed_prob_sum.mif", H);
        auto prob_mean = prob_mean_data.voxel();
        auto prob_sum = prob_sum_data.voxel();
        VoxelAccessor v (accessor);
        Image::Loop loop;
        for (loop.start (v, prob_mean, prob_sum); loop.ok(); loop.next (v, prob_mean, prob_sum)) {
          if (v.value()) {

            float sum = 0.0;
            size_t count = 0;
            for (Fixel_map<Fixel_TD_seed>::ConstIterator i = begin (v); i; ++i) {
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
        std::uniform_int_distribution<size_t> uniform_int (0, fixels.size()-2);

        while (1) {

          ++samples;
          const size_t fixel_index = 1 + uniform_int (rng.rng);
          const Fixel& fixel = fixels[fixel_index];

          if (fixel.get_seed_prob (mu()) > rng()) {

            const Point<int>& v (fixel.get_voxel());
            const Point<float> vp (v[0]+rng()-0.5, v[1]+rng()-0.5, v[2]+rng()-0.5);
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
              std::lock_guard<std::mutex> lock (mutex);
              total_samples += samples;
              ++total_seeds;
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
        seed_output.append (tck);
      }
#endif






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
          return true;
        }






      }
    }
  }
}


