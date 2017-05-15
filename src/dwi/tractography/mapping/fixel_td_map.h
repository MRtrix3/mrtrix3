/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#ifndef __dwi_tractography_mapping_fixel_td_map_h__
#define __dwi_tractography_mapping_fixel_td_map_h__


#include "dwi/fixel_map.h"
#include "dwi/directions/set.h"
#include "dwi/tractography/mapping/voxel.h"

namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Mapping
      {



        // Templated Fixel class MUST provide operator+= (const float) for adding streamline density

      template <class Fixel>
      class Fixel_TD_map : public Fixel_map<Fixel>
      { MEMALIGN(Fixel_TD_map<Fixel>)

          using MapVoxel = typename Fixel_map<Fixel>::MapVoxel;
          using VoxelAccessor = typename Fixel_map<Fixel>::VoxelAccessor;

        public:
          Fixel_TD_map (const Header& H, const DWI::Directions::FastLookupSet& directions) :
              Fixel_map<Fixel> (H),
              dirs (directions) { }
          Fixel_TD_map (const Fixel_TD_map&) = delete;

          virtual ~Fixel_TD_map() { }

          virtual bool operator() (const SetDixel& in);

        protected:
          using Fixel_map<Fixel>::accessor;
          using Fixel_map<Fixel>::fixels;

          const DWI::Directions::FastLookupSet& dirs;

          size_t dixel2fixel (const Dixel&) const;

      };





        template <class Fixel>
        bool Fixel_TD_map<Fixel>::operator() (const SetDixel& in)
        {
          for (const auto& i : in) {
            const size_t fixel_index = dixel2fixel (i);
            if (fixel_index)
              fixels[fixel_index] += i.get_length();
          }
          return true;
        }


        template <class Fixel>
        size_t Fixel_TD_map<Fixel>::dixel2fixel (const Dixel& in) const
        {
          auto v = accessor();
          assign_pos_of (in).to (v);
          if (is_out_of_bounds (v))
            return 0;
          if (!v.value())
            return 0;
          const MapVoxel& map_voxel (*v.value());
          if (map_voxel.empty())
            return 0;
          return map_voxel.dir2fixel (in.get_dir());
        }



      }
    }
  }
}


#endif
