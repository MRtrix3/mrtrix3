/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert Smith, 2012.

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
      {

          typedef typename Fixel_map<Fixel>::MapVoxel MapVoxel;
          typedef typename Fixel_map<Fixel>::VoxelAccessor VoxelAccessor;

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
