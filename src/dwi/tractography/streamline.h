/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

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

#ifndef __dwi_tractography_streamline_h__
#define __dwi_tractography_streamline_h__


#include <vector>

#include "point.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {


      template <typename T = float>
        class Streamline : public std::vector< Point<T> >
      {
        public:
          typedef T value_type;
          Streamline () : index (-1), weight (value_type (1.0)) { }

          Streamline (size_t size) : 
            std::vector< Point<value_type> > (size), 
            index (-1),
            weight (value_type (1.0)) { }

          Streamline (size_t size, const Point<float>& fill) :
            std::vector< Point<value_type> > (size, fill),
            index (-1),
            weight (value_type (1.0)) { }

          Streamline (const Streamline& that) :
            std::vector< Point<value_type> > (that),
            index (that.index),
            weight (that.weight) { }

          Streamline (Streamline&& that) :
            std::vector< Point<value_type> > (std::move (that)),
            index (that.index),
            weight (that.weight)
          {
            that.index = -1;
            that.weight = 1.0f;
          }

          Streamline (const std::vector< Point<value_type> >& tck) :
            std::vector< Point<value_type> > (tck),
            index (-1),
            weight (1.0) { }

          Streamline& operator= (Streamline&& that)
          {
            std::vector< Point<T> >::operator= (std::move (that));
            index = that.index; that.index = -1;
            weight = that.weight; that.weight = 1.0f;
            return *this;
          }

          Streamline& operator= (const Streamline& that)
          {
            std::vector< Point<float> >::operator= (that);
            index = that.index;
            weight = that.weight;
            return *this;
          }

          void clear()
          {
            std::vector< Point<T> >::clear();
            index = -1;
            weight = 1.0;
          }

          float calc_length() const;
          float calc_length (const float step_size) const;

          size_t index;
          value_type weight;
      };




      template <typename T>
      float Streamline<T>::calc_length() const
      {
        switch (Streamline<T>::size()) {
          case 0: return NAN;
          case 1: return 0.0;
          case 2: return dist ((*this)[0], (*this)[1]);
          case 3: return (dist ((*this)[1], (*this)[0]) + dist ((*this)[2], (*this)[1]));
          default: break;
        }
        const size_t midpoint = Streamline<T>::size() / 2;
        const float step_size = dist ((*this)[midpoint-1], (*this)[midpoint]);
        return calc_length (step_size);
      }

      template <typename T>
      float Streamline<T>::calc_length (const float step_size) const
      {
        switch (Streamline<T>::size()) {
          case 0: return NAN;
          case 1: return 0.0;
          case 2: return dist ((*this)[0], (*this)[1]);
          case 3: return (dist ((*this)[1], (*this)[0]) + dist ((*this)[2], (*this)[1]));
          default: break;
        }
        const size_t size = Streamline<T>::size();
        return (((size-3) * step_size) + dist ((*this)[1], (*this)[0]) + dist ((*this)[size-1], (*this)[size-2]));
      }



    }
  }
}


#endif

