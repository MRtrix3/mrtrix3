/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by Robert Smith, 2010.

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
              for (std::vector<dir_t>::const_iterator i = dirs->get_adj_dirs(d).begin(); i != dirs->get_adj_dirs(d).end(); ++i)
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
              for (std::vector<dir_t>::const_iterator i = dirs->get_adj_dirs(d).begin(); i != dirs->get_adj_dirs(d).end(); ++i)
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
        for (std::vector<dir_t>::const_iterator i = dirs->get_adj_dirs (d).begin(); i != dirs->get_adj_dirs (d).end(); ++i) {
          if (test (*i))
            return true;
        }
        return false;
      }




    }
  }
}
