/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 23/05/09.

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

#ifndef __image_misc_h__
#define __image_misc_h__

#include "types.h"
#include "datatype.h"

namespace MR
{
  namespace Image
  {

    //! returns the number of voxel in the data set, or a relevant subvolume
    template <class InfoType> 
      inline size_t voxel_count (const InfoType& in, size_t from_axis = 0, size_t to_axis = std::numeric_limits<size_t>::max())
    {
      if (to_axis > in.ndim()) to_axis = in.ndim();
      assert (from_axis < to_axis);
      size_t fp = 1;
      for (size_t n = from_axis; n < to_axis; ++n)
        fp *= in.dim (n);
      return fp;
    }

    //! returns the number of voxel in the relevant subvolume of the data set
    template <class InfoType> 
      inline size_t voxel_count (const InfoType& in, const char* specifier)
    {
      size_t fp = 1;
      for (size_t n = 0; n < in.ndim(); ++n)
        if (specifier[n] != ' ') fp *= in.dim (n);
      return fp;
    }

    //! returns the number of voxel in the relevant subvolume of the data set
    template <class InfoType> 
      inline int64_t voxel_count (const InfoType& in, const std::vector<size_t>& axes)
    {
      int64_t fp = 1;
      for (size_t n = 0; n < axes.size(); ++n) {
        assert (axes[n] < in.ndim());
        fp *= in.dim (axes[n]);
      }
      return fp;
    }

    inline int64_t footprint (int64_t count, DataType dtype) {
      return dtype == DataType::Bit ? (count+7)/8 : count*dtype.bytes();
    }

    //! returns the memory footprint of a DataSet
    template <class InfoType> 
      inline int64_t footprint (const InfoType& in, size_t from_dim = 0, size_t up_to_dim = std::numeric_limits<size_t>::max()) {
      return footprint (Image::voxel_count (in, from_dim, up_to_dim), in.datatype());
    }

    //! returns the memory footprint of a DataSet
    template <class InfoType> 
      inline int64_t footprint (const InfoType& in, const char* specifier) {
      return footprint (Image::voxel_count (in, specifier), in.datatype());
    }


    //! \cond skip
    namespace
    {
      template <typename T> inline bool is_complex__ ()
      {
        return false;
      }
      template <> inline bool is_complex__<cfloat> ()
      {
        return true;
      }
      template <> inline bool is_complex__<cdouble> ()
      {
        return true;
      }
    }
    //! \endcond




    //! return whether the InfoType contains complex data
    template <class InfoType> 
      inline bool is_complex (const InfoType&)
    {
      typedef typename InfoType::value_type T;
      return is_complex__<T> ();
    }

    template <class InfoType1, class InfoType2> 
      inline bool dimensions_match (const InfoType1& in1, const InfoType2& in2)
    {
      if (in1.ndim() != in2.ndim()) return false;
      for (size_t n = 0; n < in1.ndim(); ++n)
        if (in1.dim (n) != in2.dim (n)) return false;
      return true;
    }

    template <class InfoType1, class InfoType2> 
      inline bool dimensions_match (const InfoType1& in1, const InfoType2& in2, size_t from_axis, size_t to_axis)
    {
      assert (from_axis < to_axis);
      if (to_axis > in1.ndim() || to_axis > in2.ndim()) return false;
      for (size_t n = from_axis; n < to_axis; ++n)
        if (in1.dim (n) != in2.dim (n)) return false;
      return true;
    }

    template <class InfoType1, class InfoType2> 
      inline bool dimensions_match (const InfoType1& in1, const InfoType2& in2, const std::vector<size_t>& axes)
    {
      for (size_t n = 0; n < axes.size(); ++n) {
        if (in1.ndim() <= axes[n] || in2.ndim() <= axes[n]) return false;
        if (in1.dim (axes[n]) != in2.dim (axes[n])) return false;
      }
      return true;
    }

    template <class InfoType1, class InfoType2> 
      inline void check_dimensions (const InfoType1& in1, const InfoType2& in2)
    {
      if (!dimensions_match (in1, in2))
        throw Exception ("dimension mismatch between \"" + in1.name() + "\" and \"" + in2.name() + "\"");
    }

    template <class InfoType1, class InfoType2> 
      inline void check_dimensions (const InfoType1& in1, const InfoType2& in2, size_t from_axis, size_t to_axis)
    {
      if (!dimensions_match (in1, in2, from_axis, to_axis))
        throw Exception ("dimension mismatch between \"" + in1.name() + "\" and \"" + in2.name() + "\"");
    }

    template <class InfoType1, class InfoType2> 
      inline void check_dimensions (const InfoType1& in1, const InfoType2& in2, const std::vector<size_t>& axes)
    {
      if (!dimensions_match (in1, in2, axes))
        throw Exception ("dimension mismatch between \"" + in1.name() + "\" and \"" + in2.name() + "\"");
    }



    template <class InfoType>
      inline void squeeze_dim (InfoType& in, size_t from_axis = 3) 
      {
        size_t n = in.ndim();
        while (in.dim(n-1) <= 1 && n > from_axis) --n;
        in.set_ndim (n);
      }


    //! \cond skip
    namespace {

      template <class VoxelType>
        struct __assign_pos_axis_range
        {
          template <class VoxelType2>
            void operator() (VoxelType2& out) const {
              const size_t max_axis = std::min (to_axis, std::min (ref.ndim(), out.ndim()));
              for (size_t n = from_axis; n < max_axis; ++n)
                out[n] = ref[n];
            }
          const VoxelType& ref;
          const size_t from_axis, to_axis;
        };


      template <class VoxelType, typename IntType>
        struct __assign_pos_axes
        {
          template <class VoxelType2>
            void operator() (VoxelType2& out) const {
              for (auto a : axes) 
                out[a] = ref[a];
            }
          const VoxelType& ref;
          const std::vector<IntType> axes;
        };
    }

    //! \endcond


    //! returns a functor to set the position in ref to other voxels
    template <class VoxelType>
      inline __assign_pos_axis_range<VoxelType> 
      assign_pos (const VoxelType& reference, size_t from_axis = 0, size_t to_axis = std::numeric_limits<size_t>::max()) 
      {
        return { reference, from_axis, to_axis };
      }

    //! returns a functor to set the position in ref to other voxels
    template <class VoxelType, typename IntType>
      inline __assign_pos_axes<VoxelType, IntType> 
      assign_pos (const VoxelType& reference, const std::vector<IntType>& axes) 
      {
        return { reference, axes };
      }

    //! returns a functor to set the position in ref to other voxels
    template <class VoxelType, typename IntType>
      inline __assign_pos_axes<VoxelType, IntType> 
      assign_pos (const VoxelType& reference, const std::vector<IntType>&& axes) 
      {
        return assign_pos (reference, axes);
      }


  }
}

#endif

