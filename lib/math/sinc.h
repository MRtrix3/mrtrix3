/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 12/08/11.

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

#ifndef __math_sinc_h__
#define __math_sinc_h__


namespace MR
{
  namespace Math
  {

    template <typename T> class Sinc
    {
      public:
        typedef T value_type;

        Sinc (const size_t w) :
          window_size (w),
          max_offset_from_kernel_centre ((w-1) / 2),
          indices (w),
          weights (w),
          current_pos (NAN)
        {
          assert (w % 2);
        }

        template <class Set>
        void set (const Set& set, const size_t axis, const value_type position) {

          if (position == current_pos)
            return;

          const int kernel_centre = round (position);
          value_type sum_weights = 0.0;

          for (size_t i = 0; i != window_size; ++i) {

            const int voxel = kernel_centre - max_offset_from_kernel_centre + i;
            if (voxel < 0)
              indices[i] = -voxel - 1;
            else if (voxel >= set.dim (axis))
              indices[i] = (2 * int(set.dim (axis))) - voxel - 1;
            else
              indices[i] = voxel;

            const value_type offset = position - value_type (voxel);

            const value_type sinc           = offset ? Math::sin (M_PI * offset) / (M_PI * offset) : 1.0;
            const value_type hamming_factor = 0.5 * (1.0 + Math::cos (M_PI * offset / (max_offset_from_kernel_centre + 1)));
            const value_type this_weight    = hamming_factor * sinc;

            weights[i]  =  this_weight;
            sum_weights += this_weight;

          }

          const value_type normalisation = 1.0 / sum_weights;
          for (size_t i = 0; i != window_size; ++i)
            weights[i] *= normalisation;

          current_pos = position;

        }

        size_t index (const size_t i) const { return indices[i]; }

        template <class Set>
        value_type value (Set& set, const size_t axis) const {
          assert (current_pos != NAN);
          const size_t init_pos = set[axis];
          value_type sum = 0.0;
          for (size_t i = 0; i != window_size; ++i) {
            set[axis] = indices[i];
            sum += set.value() * weights[i];
          }
          set[axis] = init_pos;
          return sum;
        }

        template <class Cont>
        value_type value (Cont& data) const {
          assert (data.size() == window_size);
          assert (current_pos != NAN);
          value_type sum = 0.0;
          for (size_t i = 0; i != window_size; ++i)
            sum += data[i] * weights[i];
          return sum;
        }

        value_type value (value_type* data) const {
          assert (current_pos != NAN);
          value_type sum = 0.0;
          for (size_t i = 0; i != window_size; ++i)
            sum += data[i] * weights[i];
          return sum;
        }

      private:
        const size_t window_size, max_offset_from_kernel_centre;
        std::vector<size_t> indices;
        std::vector<value_type> weights;
        value_type  current_pos;

    };

  }
}

#endif
