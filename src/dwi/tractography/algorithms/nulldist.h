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

#ifndef __dwi_tractography_algorithms_nulldist_h__
#define __dwi_tractography_algorithms_nulldist_h__

#include "dwi/tractography/algorithms/iFOD2.h"
#include "dwi/tractography/tracking/method.h"
#include "dwi/tractography/tracking/shared.h"
#include "dwi/tractography/tracking/types.h"



namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Algorithms
      {

    using namespace MR::DWI::Tractography::Tracking;

    class NullDist1 : public MethodBase { MEMALIGN(NullDist1)
      public:

      class Shared : public SharedBase { MEMALIGN(Shared)
        public:
        Shared (const std::string& diff_path, DWI::Tractography::Properties& property_set) :
          SharedBase (diff_path, property_set)
        {
          set_step_and_angle (rk4 ? Defaults::stepsize_voxels_rk4 : Defaults::stepsize_voxels_firstorder,
                              Defaults::angle_ifod1,
                              rk4);
          set_num_points();
          set_cutoff (0.0f);
          sin_max_angle_1o = std::sin (max_angle_1o);
          properties["method"] = "Nulldist1";
        }
        float sin_max_angle_1o;
      };

      NullDist1 (const Shared& shared) :
        MethodBase (shared),
        S (shared),
        source (S.source) { }


      bool init() override {
        if (!get_data (source))
          return false;
        dir = S.init_dir.allFinite() ? S.init_dir : random_direction();
        return true;
      }

      term_t next () override {
        if (!get_data (source))
          return EXIT_IMAGE;
        dir = rand_dir (dir);
        dir.normalize();
        pos += S.step_size * dir;
        return CONTINUE;
      }

      float get_metric (const Eigen::Vector3f&, const Eigen::Vector3f&) override { return uniform(rng); }


      protected:
      const Shared& S;
      Interpolator<Image<float>>::type source;

      Eigen::Vector3f rand_dir (const Eigen::Vector3f& d) { return (random_direction (d, S.max_angle_1o, S.sin_max_angle_1o)); }

    };

    class NullDist2 : public iFOD2 { MEMALIGN(NullDist2)
      public:

      class Shared : public iFOD2::Shared { MEMALIGN(Shared)
        public:
        Shared (const std::string& diff_path, DWI::Tractography::Properties& property_set) :
          iFOD2::Shared (diff_path, property_set)
        {
          set_cutoff (0.0f);
          properties["method"] = "Nulldist2";
        }
      };

      NullDist2 (const Shared& shared) :
        iFOD2 (shared),
        S (shared),
        source (S.source),
        positions (S.num_samples),
        tangents (S.num_samples),
        sample_idx (S.num_samples) { }

      NullDist2 (const NullDist2& that) :
        iFOD2 (that),
        S (that.S),
        source (S.source),
        positions (S.num_samples),
        tangents (S.num_samples),
        sample_idx (S.num_samples) { }

      bool init() override {
        if (!get_data (source))
          return false;
        dir = S.init_dir.allFinite() ? S.init_dir : random_direction();
        sample_idx = S.num_samples;
        return true;
      }

      term_t next () override {

        if (++sample_idx < S.num_samples) {
          pos = positions[sample_idx];
          dir = tangents [sample_idx];
          return CONTINUE;
        }

        iFOD2::get_path (positions, tangents, iFOD2::rand_dir (dir));
        if (S.is_act()) {
          if (!act().fetch_tissue_data (positions[S.num_samples - 1]))
            return EXIT_IMAGE;
        }
        pos = positions[0];
        dir = tangents[0];
        sample_idx = 0;
        return CONTINUE;

      }

      void reverse_track() override
      {
        sample_idx = S.num_samples;
        MethodBase::reverse_track();
      }

      void truncate_track (GeneratedTrack& tck, const size_t length_to_revert_from, const size_t revert_step) override
      {
        iFOD2::truncate_track (tck, length_to_revert_from, revert_step);
        sample_idx = S.num_samples;
      }

      float get_metric (const Eigen::Vector3f&, const Eigen::Vector3f&) override { return uniform(rng); }


      protected:
      const Shared& S;
      Interpolator<Image<float>>::type source;

      vector<Eigen::Vector3f> positions, tangents;
      size_t sample_idx;

    };

      }
    }
  }
}

#endif


