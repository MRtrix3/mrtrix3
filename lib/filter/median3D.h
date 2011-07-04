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

#ifndef __filter_median3D_h__
#define __filter_median3D_h__

#include "dataset/kernel.h"
#include "filter/base.h"

namespace MR
{
  namespace Filter
  {
    namespace
    {
      template <typename T> class MedianFunctor
      {
        public:
          MedianFunctor (const std::vector<int>& extent)
          {
            if (extent.size() == 1)
              dim[0] = dim[1] = dim[2] = extent[0];
            else {
              dim[0] = extent[0];
              dim[1] = extent[1];
              dim[2] = extent[2];
            }
            allocate();
          }

          MedianFunctor (const MedianFunctor& F)
          {
            dim[0] = F.dim[0];
            dim[1] = F.dim[1];
            dim[2] = F.dim[2];
            allocate();
          }

          ssize_t extent (size_t axis) const { return (dim[axis]); }

          template <class Set>
            void prepare (Set& set, size_t x_axis, size_t y_axis, size_t z_axis) { }

          T operator() (const DataSet::Kernel::Data<T>& kernel) const
          {
            size_t nc = 0;
            T cm = -INFINITY;
            const size_t n = kernel.count();
            const size_t m = n/2 + 1;

            for (ssize_t k = kernel.from(2); k < kernel.to(2); ++k) {
              for (ssize_t j = kernel.from(1); j < kernel.to(1); ++j) {
                for (ssize_t i = kernel.from(0); i < kernel.to(0); ++i) {
                  const T val = kernel (i,j,k);
                  if (nc < m) {
                    v[nc] = val;
                    if (v[nc] > cm) cm = val;
                    ++nc;
                  }
                  else if (val < cm) {
                    size_t x;
                    for (x = 0; v[x] != cm; ++x);
                    v[x] = val;
                    cm = -INFINITY;
                    for (x = 0; x < m; x++)
                      if (v[x] > cm) cm = v[x];
                  }
                }
              }
            }

            if ((n+1) & 1) {
              T t = cm = -INFINITY;
              for (size_t i = 0; i < m; ++i) {
                if (v[i] > cm) {
                  t = cm;
                  cm = v[i];
                }
                else if (v[i] > t) t = v[i];
              }
              cm = (cm+t)/2.0;
            }

            return (cm);
          }

        private:
          ssize_t dim[3];
          Ptr<T,true> v;

          void allocate ()
          {
            v = new T [(dim[0]*dim[1]*dim[2]+1)/2];
          }
      };
    }

    /** \addtogroup Filters
    @{ */

    //! Smooth images using median filtering.
    template <class InputSet, class OutputSet> class Median3DFilter :
      public Base<InputSet, OutputSet>
    {

    public:
        Median3DFilter (InputSet& DataSet) :
          Base<InputSet, OutputSet>(DataSet),
          extent_(1) {
          input_image_ = &DataSet;
          extent_[0] = 3;
        }

        void set_extent (const std::vector<int>& extent) {
          for (size_t i = 0; i < extent.size(); ++i)
            if (! (extent[i] & int(1)))
              throw Exception ("expected odd number for extent");
          if (extent.size() != 1 && extent.size() != 3)
            throw Exception ("unexpected number of elements specified in extent");
          extent_.resize(extent.size());
          for (size_t i = 0; i < extent.size(); ++i)
            extent_[i] = extent[i];
        }

        void execute (OutputSet& output) {
          DataSet::Kernel::run (output, *input_image_, MedianFunctor<float>(extent_), "median filtering...");
        }

    private:
      InputSet* input_image_;
      std::vector<int> extent_;
    };
    //! @}
  }
}



#endif
