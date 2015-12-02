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

#ifndef __dwi_tractography_algorithms_nulldist_h__
#define __dwi_tractography_algorithms_nulldist_h__

#include "point.h"
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

    class NullDist1 : public MethodBase {
      public:

      class Shared : public SharedBase {
        public:
        Shared (const std::string& diff_path, DWI::Tractography::Properties& property_set) :
          SharedBase (diff_path, property_set)
        {
          set_step_size (0.1);
          sin_max_angle = std::sin (max_angle);
          properties["method"] = "Nulldist1";
        }
        value_type sin_max_angle;
      };

      NullDist1 (const Shared& shared) :
        MethodBase (shared),
        S (shared),
        source (S.source_voxel) { }


      bool init() {
        if (!get_data (source))
          return false;
        dir = S.init_dir.valid() ? S.init_dir : random_direction();
        return true;
      }

      term_t next () {
        if (!get_data (source))
          return EXIT_IMAGE;
        dir = rand_dir (dir);
        dir.normalise();
        pos += S.step_size * dir;
        return CONTINUE;
      }

      float get_metric() { return uniform_rng(); }


      protected:
      const Shared& S;
      Interpolator<SourceBufferType::voxel_type>::type source;

      Point<value_type> rand_dir (const Point<value_type>& d) { return (random_direction (d, S.max_angle, S.sin_max_angle)); }

    };

    class NullDist2 : public iFOD2 {
      public:

      class Shared : public iFOD2::Shared {
        public:
        Shared (const std::string& diff_path, DWI::Tractography::Properties& property_set) :
          iFOD2::Shared (diff_path, property_set)
        {
          properties["method"] = "Nulldist2";
        }
      };

      NullDist2 (const Shared& shared) :
        iFOD2 (shared),
        S (shared),
        source (S.source_voxel),
        positions (S.num_samples),
        tangents (S.num_samples),
        sample_idx (S.num_samples) { }

      NullDist2 (const NullDist2& that) :
        iFOD2 (that),
        S (that.S),
        source (S.source_voxel),
        positions (S.num_samples),
        tangents (S.num_samples),
        sample_idx (S.num_samples) { }

      bool init() {
        if (!get_data (source))
          return false;
        dir = S.init_dir.valid() ? S.init_dir : random_direction();
        sample_idx = S.num_samples;
        return true;
      }

      term_t next () {

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

      void truncate_track (GeneratedTrack& tck, const size_t length_to_revert_from, const size_t revert_step)
      {
        iFOD2::truncate_track (tck, length_to_revert_from, revert_step);
        sample_idx = S.num_samples;
      }

      float get_metric() { return uniform_rng(); }


      protected:
      const Shared& S;
      Interpolator<SourceBufferType::voxel_type>::type source;

      std::vector< Point<value_type> > positions, tangents;
      size_t sample_idx;

    };

      }
    }
  }
}

#endif


