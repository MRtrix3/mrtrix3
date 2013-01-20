/*
   Copyright 2008 Brain Research Institute, Melbourne, Australia

   Written by Robert Smith, 2011.

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




#ifndef __dwi_directions_set_h__
#define __dwi_directions_set_h__



#include <stdint.h>
#include <vector>

#include "point.h"
#include "progressbar.h"

#include "dwi/directions/predefined.h"
#include "math/matrix.h"




namespace MR {
  namespace DWI {
    namespace Directions {



      typedef unsigned int dir_t;



      class Set {

        public:

          explicit Set (const std::string& path) :
              dir_mask_bytes (0),
              dir_mask_excess_bits (0),
              dir_mask_excess_bits_mask (0)
          {
            Math::Matrix<float> az_el_pairs;
            az_el_pairs.load (path);

            if (az_el_pairs.columns() != 2)
              throw Exception ("Text file \"" + path + "\"does not contain directions as azimuth-elevation pairs");

            initialise (az_el_pairs);
          }

          explicit Set (const size_t d) :
              dir_mask_bytes (0),
              dir_mask_excess_bits (0),
              dir_mask_excess_bits_mask (0)
          {
            Math::Matrix<float> az_el_pairs;
            load_predefined (az_el_pairs, d);
            initialise (az_el_pairs);
          }

          virtual ~Set();

          size_t size () const { return unit_vectors.size(); }
          const Point<float>& get_dir (const size_t i) const { return unit_vectors[i]; }
          const std::vector<dir_t>& get_adj_dirs (const size_t i) const { return adj_dirs[i]; }

          bool dirs_are_adjacent (const dir_t one, const dir_t two) const {
            for (std::vector<dir_t>::const_iterator i = adj_dirs[one].begin(); i != adj_dirs[one].end(); ++i) {
              if (*i == two)
                return true;
            }
            return false;
          }

          dir_t get_min_linkage (const dir_t one, const dir_t two) const;

          const std::vector< Point<float> >& get_dirs() const { return unit_vectors; }
          const Point<float>& operator[] (const size_t i) const { return unit_vectors[i]; }


        protected:

          std::vector< Point<float> > unit_vectors;
          std::vector<dir_t>* adj_dirs; // Note: not self-inclusive


        private:

          size_t dir_mask_bytes, dir_mask_excess_bits;
          uint8_t dir_mask_excess_bits_mask;
          friend class Mask;

          Set ();

          void load_predefined (Math::Matrix<float>& az_el_pairs, const size_t);
          void initialise (const Math::Matrix<float>& az_el_pairs);

      };






      class FastLookupSet : public Set {

        public:

          FastLookupSet (const std::string& path) : 
              Set (path)
          {
            initialise();
          }

          FastLookupSet (const size_t d) :
              Set (d)
          {
            initialise();
          }

          FastLookupSet (const FastLookupSet&);
          ~FastLookupSet ();

          dir_t select_direction (const Point<float>&) const;



        private:

          std::vector<dir_t>* grid_lookup;
          unsigned int num_az_grids, num_el_grids, total_num_angle_grids;
          float az_grid_step, el_grid_step;
          float az_begin, el_begin;

          FastLookupSet ();

          dir_t select_direction_slow (const Point<float>&) const;

          void initialise();

          size_t dir2gridindex (const Point<float>&) const;

          void test_lookup() const;

      };



    }
  }
}

#endif

