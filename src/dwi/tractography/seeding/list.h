/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#ifndef __dwi_tractography_seeding_list_h__
#define __dwi_tractography_seeding_list_h__


#include "types.h"

#include "dwi/tractography/seeding/base.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Seeding
      {




      class List
      { MEMALIGN(List)

        public:
          List() :
            total_volume (0.0),
            total_count (0) { }

          List (const List&) = delete;


          void add (Base* const in);
          void clear();
          bool get_seed (Eigen::Vector3f& p, Eigen::Vector3f& d);


          size_t num_seeds() const { return seeders.size(); }
          const Base* operator[] (const size_t n) const { return seeders[n].get(); }
          bool is_finite() const { return total_count; }
          uint32_t get_total_count() const { return total_count; }


          friend inline std::ostream& operator<< (std::ostream& stream, const List& S) {
            if (S.seeders.empty())
              return stream;
            auto i = S.seeders.cbegin();
            stream << **i;
            for (++i; i != S.seeders.cend(); ++i)
              stream << ", " << **i;
            return (stream);
          }


        private:
          vector<std::unique_ptr<Base>> seeders;
          float total_volume;
          uint32_t total_count;

      };





      }
    }
  }
}

#endif

