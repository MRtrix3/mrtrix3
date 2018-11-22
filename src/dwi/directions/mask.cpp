/* Copyright (c) 2008-2019 the MRtrix3 contributors.
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
              for (vector<index_type>::const_iterator i = dirs->get_adj_dirs(d).begin(); i != dirs->get_adj_dirs(d).end(); ++i)
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
              for (vector<index_type>::const_iterator i = dirs->get_adj_dirs(d).begin(); i != dirs->get_adj_dirs(d).end(); ++i)
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
        for (vector<index_type>::const_iterator i = dirs->get_adj_dirs (d).begin(); i != dirs->get_adj_dirs (d).end(); ++i) {
          if (test (*i))
            return true;
        }
        return false;
      }




    }
  }
}
