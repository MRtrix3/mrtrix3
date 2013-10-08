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

#ifndef __dwi_tractography_seeding_list_h__
#define __dwi_tractography_seeding_list_h__


#include "math/rng.h"

#include "dwi/tractography/seeding/base.h"

#include <vector>



namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Seeding
      {




      class List
      {

        public:
          List() :
            total_volume (0.0),
            total_count (0) { }

          ~List()
          {
            for (std::vector<Base*>::iterator i = seeders.begin(); i != seeders.end(); ++i) {
              delete *i;
              *i = NULL;
            }
          }


          void add (Base* const in);
          void clear();
          bool get_seed (Point<float>& p, Point<float>& d);


          size_t num_seeds() const { return seeders.size(); }
          const Base* operator[] (const size_t n) const { return seeders[n]; }
          bool is_finite() const { return total_count; }
          uint32_t get_total_count() const { return total_count; }
          const Math::RNG& get_rng() const { return rng; }


          friend inline std::ostream& operator<< (std::ostream& stream, const List& S) {
            if (S.seeders.empty())
              return stream;
            std::vector<Base*>::const_iterator i = S.seeders.begin();
            stream << **i;
            for (++i; i != S.seeders.end(); ++i)
              stream << ", " << **i;
            return (stream);
          }


        private:
          std::vector<Base*> seeders;
          Math::RNG rng;
          float total_volume;
          uint32_t total_count;

      };





      }
    }
  }
}

#endif

