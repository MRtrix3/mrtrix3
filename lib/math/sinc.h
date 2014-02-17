/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


#ifndef __math_sinc_h__
#define __math_sinc_h__


namespace MR
{
  namespace Math
  {

    template <typename T = float> class Sinc
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

            const value_type offset        = position - (value_type)voxel;

            const value_type sinc          = offset ? Math::sin (M_PI * offset) / (M_PI * offset) : 1.0;

            //const value_type hann_cos_term = M_PI * offset / (value_type(max_offset_from_kernel_centre) + 0.5);
            //const value_type hann_factor   = (fabs (hann_cos_term) < M_PI) ? 0.5 * (1.0 + Math::cos (hann_cos_term)) : 0.0;
            //const value_type this_weight   = hann_factor * sinc;

            const value_type lanczos_sinc_term = fabs (M_PI * offset / (double(max_offset_from_kernel_centre) + 0.5));
            value_type lanczos_factor = 0.0;
            if (lanczos_sinc_term < M_PI) {
              if (lanczos_sinc_term)
                lanczos_factor = Math::sin (lanczos_sinc_term) / lanczos_sinc_term;
              else
                lanczos_factor = 1.0;
            }
            const value_type this_weight = lanczos_factor * sinc;

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
