/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 25/10/09.

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

#ifndef __dwi_tractography_algorithms_tensor_prob_h__
#define __dwi_tractography_algorithms_tensor_prob_h__

#include "dwi/bootstrap.h"
#include "dwi/tractography/algorithms/tensor_det.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Algorithms
      {

      using namespace MR::DWI::Tractography::Tracking;

      class Tensor_Prob : public Tensor_Det {
        public:
          class Shared : public Tensor_Det::Shared {
            public:
              Shared (const std::string& diff_path, DWI::Tractography::Properties& property_set) :
                Tensor_Det::Shared (diff_path, property_set)
              {

                if (is_act() && act().backtrack())
                  throw Exception ("Sorry, backtracking not currently enabled for TensorProb algorithm");

                properties["method"] = "TensorProb";

                mult (Hat, bmat, binv);
              }

              Math::Matrix<value_type> Hat;
          };






          Tensor_Prob (const Shared& shared) :
            Tensor_Det (shared),
            S (shared),
            source (Bootstrap<SourceBufferType::voxel_type,WildBootstrap> (S.source_voxel, WildBootstrap (S.Hat, rng))) { }

          Tensor_Prob (const Tensor_Prob& F) :
            Tensor_Det (F.S),
            S (F.S),
            source (Bootstrap<SourceBufferType::voxel_type,WildBootstrap> (S.source_voxel, WildBootstrap (S.Hat, rng))) { }



          bool init() {
            source.clear();
            if (!source.get (pos, values))
              return false;
            return Tensor_Det::do_init();
          }



          term_t next () {
            if (!source.get (pos, values))
              return EXIT_IMAGE;
            return Tensor_Det::do_next();
          }


/*
          void truncate_track (Track& tck, const int revert_step)
          {

            // Need to erase the relevant entries in wb_set
            // "revert_step" is interpreted as the number of voxels for which WB data must be re-acquired
            // Beware though; each point corresponds to a 2x2x2 linear interpolation neighbourhood
            // As such, would need to remove all points which have sampled from the WB signal in that voxel

            // Bugger it - just do it per-point, not worth the effort

            // But then, will have to further truncate to remove any points that sampled from these voxels during interpolation!

            //fprintf (stderr, "\n\nNew truncation, size %d, current track:\n", revert_step);
            //for (Track::const_iterator i = tck.begin(); i != tck.end(); ++i)
            //  fprintf (stderr, "[%f %f %f]\n", (*i)[0], (*i)[1], (*i)[2]);

            const int new_end_idx = tck.size() - 1 - revert_step;
            if (new_end_idx <= 1) {
              tck.clear();
              return;
            }

            //fprintf (stderr, "\nNew end index: %d\n", new_end_idx);

            for (int i = new_end_idx + 1; i != int(tck.size()); ++i) {
              const Point<float> v (source.scanner2voxel (tck[i]));
              const Point<ssize_t> lower_voxel (Math::floor (v[0]), Math::floor (v[1]), Math::floor (v[2]));
              unsigned int erase_count = 0;
              erase_count += (wb_set.erase (lower_voxel + Point<ssize_t> (0, 0, 0))) ? 1 : 0;
              erase_count += (wb_set.erase (lower_voxel + Point<ssize_t> (0, 0, 1))) ? 1 : 0;
              erase_count += (wb_set.erase (lower_voxel + Point<ssize_t> (0, 1, 0))) ? 1 : 0;
              erase_count += (wb_set.erase (lower_voxel + Point<ssize_t> (0, 1, 1))) ? 1 : 0;
              erase_count += (wb_set.erase (lower_voxel + Point<ssize_t> (1, 0, 0))) ? 1 : 0;
              erase_count += (wb_set.erase (lower_voxel + Point<ssize_t> (1, 0, 1))) ? 1 : 0;
              erase_count += (wb_set.erase (lower_voxel + Point<ssize_t> (1, 1, 0))) ? 1 : 0;
              erase_count += (wb_set.erase (lower_voxel + Point<ssize_t> (1, 1, 1))) ? 1 : 0;
              //fprintf (stderr, "Erased point [%f %f %f], lower voxel [%d %d %d], %d voxel erases\n", tck[i][0], tck[i][1], tck[i][2], lower_voxel[0], lower_voxel[1], lower_voxel[2], erase_count);
            }

            tck.erase (tck.begin() + new_end_idx + 1, tck.end());

          }
*/
        void truncate_track (std::vector< Point<value_type> >& tck, const int revert_step) {}


        protected:


          class WildBootstrap {
            public:
              WildBootstrap (const Math::Matrix<value_type>& hat_matrix, Math::RNG& random_number_generator) :
                H (hat_matrix),
                rng (random_number_generator),
                residuals (H.rows()),
                log_signal (H.rows()) { }

              void operator() (value_type* data) {
                for (size_t i = 0; i < residuals.size(); ++i)
                  log_signal[i] = data[i] > value_type (0.0) ? -Math::log (data[i]) : value_type (0.0);

                mult (residuals, H, log_signal);

                for (size_t i = 0; i < residuals.size(); ++i) {
                  residuals[i] = Math::exp (-residuals[i]) - data[i];
                  data[i] += rng.uniform_int (2) ? residuals[i] : -residuals[i];
                }
              }

            private:
              const Math::Matrix<value_type>& H;
              Math::RNG& rng;
              Math::Vector<value_type> residuals, log_signal;
          };


          class Interp : public Interpolator<Bootstrap<SourceBufferType::voxel_type,WildBootstrap> >::type {
            public:
              Interp (const Bootstrap<SourceBufferType::voxel_type,WildBootstrap>& bootstrap_vox) :
                Interpolator<Bootstrap<SourceBufferType::voxel_type,WildBootstrap> >::type (bootstrap_vox) {
                  for (size_t i = 0; i < 8; ++i)
                    raw_signals.push_back (new value_type [dim (3)]);
                }

              VecPtr<value_type,true> raw_signals;

              bool get (const Point<value_type>& pos, std::vector<value_type>& data) {
                scanner (pos);

                if (out_of_bounds) {
                  std::fill (data.begin(), data.end(), NAN);
                  return false;
                }

                std::fill (data.begin(), data.end(), 0.0);

                if (faaa) {
                  get_values (raw_signals[0]); // aaa
                  for (size_t i = 0; i < data.size(); ++i)
                    data[i] +=  faaa * raw_signals[0][i];
                }

                ++(*this)[2];
                if (faab) {
                  get_values (raw_signals[1]); // aab
                  for (size_t i = 0; i < data.size(); ++i)
                    data[i] +=  faab * raw_signals[1][i];
                }

                ++(*this)[1];
                if (fabb) {
                  get_values (raw_signals[3]); // abb
                  for (size_t i = 0; i < data.size(); ++i)
                    data[i] +=  fabb * raw_signals[3][i];
                }

                --(*this)[2];
                if (faba) {
                  get_values (raw_signals[2]); // aba
                  for (size_t i = 0; i < data.size(); ++i)
                    data[i] +=  faba * raw_signals[2][i];
                }

                ++(*this)[0];
                if (fbba) {
                  get_values (raw_signals[6]); // bba
                  for (size_t i = 0; i < data.size(); ++i)
                    data[i] +=  fbba * raw_signals[6][i];
                }

                --(*this)[1];
                if (fbaa) {
                  get_values (raw_signals[4]); // baa
                  for (size_t i = 0; i < data.size(); ++i)
                    data[i] +=  fbaa * raw_signals[4][i];
                }

                ++(*this)[2];
                if (fbab) {
                  get_values (raw_signals[5]); // bab
                  for (size_t i = 0; i < data.size(); ++i)
                    data[i] +=  fbab * raw_signals[5][i];
                }

                ++(*this)[1];
                if (fbbb) {
                  get_values (raw_signals[7]); // bbb
                  for (size_t i = 0; i < data.size(); ++i)
                    data[i] +=  fbbb * raw_signals[7][i];
                }

                --(*this)[0];
                --(*this)[1];
                --(*this)[2];

                return !isnan (data[0]);
              }
          };

          const Shared& S;
          Interp source;


      };

      }
    }
  }
}

#endif



