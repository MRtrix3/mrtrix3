/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */


#include "dwi/directions/mask.h"



namespace MR {
  namespace DWI {
    namespace Directions {



      void Mask::erode (const size_t iterations)
      {
        for (size_t iter = 0; iter != iterations; ++iter) {
          Mask temp (*this);
          for (size_t d = 0; d != size(); ++d) {
            if (!temp[d]) {
              for (std::vector<index_type>::const_iterator i = dirs->get_adj_dirs(d).begin(); i != dirs->get_adj_dirs(d).end(); ++i)
                reset (*i);
            }
          }
        }
      }



      void Mask::dilate (const size_t iterations)
      {
        for (size_t iter = 0; iter != iterations; ++iter) {
          Mask temp (*this);
          for (size_t d = 0; d != size(); ++d) {
            if (temp[d]) {
              for (std::vector<index_type>::const_iterator i = dirs->get_adj_dirs(d).begin(); i != dirs->get_adj_dirs(d).end(); ++i)
                set (*i);
            }
          }
        }
      }



      size_t Mask::get_min_linkage (const Mask& that)
      {
        size_t iterations = 0;
        for (Mask temp (that); !(temp & *this).empty(); ++iterations)
          temp.dilate();
        return iterations;
      }



      bool Mask::is_adjacent (const size_t d) const
      {
        for (std::vector<index_type>::const_iterator i = dirs->get_adj_dirs (d).begin(); i != dirs->get_adj_dirs (d).end(); ++i) {
          if (test (*i))
            return true;
        }
        return false;
      }




    }
  }
}
