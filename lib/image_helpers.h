/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 07/01/10.

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

#ifndef __image_helpers_h__
#define __image_helpers_h__

#include "datatype.h"

namespace MR
{


  //! \cond skip
  namespace {

    template <class... DestImageType>
      struct __assign {
        __assign (size_t axis, ssize_t index) : axis (axis), index (index) { }
        const size_t axis;
        const ssize_t index;
        template <class ImageType> 
          void operator() (ImageType& x) { x.index (axis) = index; }
      };

    template <class... DestImageType>
      struct __assign<std::tuple<DestImageType...>> {
        __assign (size_t axis, ssize_t index) : axis (axis), index (index) { }
        const size_t axis;
        const ssize_t index;
        template <class ImageType> 
          void operator() (ImageType& x) { apply (__assign<DestImageType...> (axis, index), x); }
      };

    template <class... DestImageType>
      struct __max_axis {
        __max_axis (size_t& axis) : axis (axis) { }
        size_t& axis;
        template <class ImageType> 
          void operator() (ImageType& x) { if (axis > x.ndim()) axis = x.ndim(); }
      };

    template <class... DestImageType>
      struct __max_axis<std::tuple<DestImageType...>> {
        __max_axis (size_t& axis) : axis (axis) { }
        size_t& axis;
        template <class ImageType> 
          void operator() (ImageType& x) { apply (__max_axis<DestImageType...> (axis), x); }
      };

    template <class ImageType>
      struct __assign_pos_axis_range
      {
        template <class... DestImageType>
          void to (DestImageType&... dest) const {
            size_t last_axis = to_axis;
            apply (__max_axis<DestImageType...> (last_axis), std::tie (dest...));
            for (size_t n = from_axis; n < last_axis; ++n)
              apply (__assign<DestImageType...> (n, ref.index(n)), std::tie (dest...));
          }
        const ImageType& ref;
        const size_t from_axis, to_axis;
      };


    template <class ImageType, typename IntType>
      struct __assign_pos_axes
      {
        template <class... DestImageType>
          void to (DestImageType&... dest) const {
            for (auto a : axes) 
              apply (__assign<DestImageType...> (a, ref.index(a)), std::tie (dest...));
          }
        const ImageType& ref;
        const std::vector<IntType> axes;
      };
  }

  //! \endcond



  //! returns a functor to set the position in ref to other voxels
  /*! this can be used as follows:
   * \code
   * assign_pos_of (src_image, 0, 3).to (dest_image1, dest_image2);
   * \endcode */
  template <class ImageType>
    inline __assign_pos_axis_range<ImageType> 
    assign_pos_of (const ImageType& reference, size_t from_axis = 0, size_t to_axis = std::numeric_limits<size_t>::max()) 
    {
      return { reference, from_axis, to_axis };
    }

  //! returns a functor to set the position in ref to other voxels
  /*! this can be used as follows:
   * \code
   * std::vector<int> axes = { 0, 3, 4 };
   * assign_pos (src_image, axes) (dest_image1, dest_image2);
   * \endcode */
  template <class ImageType, typename IntType>
    inline __assign_pos_axes<ImageType, IntType> 
    assign_pos_of (const ImageType& reference, const std::vector<IntType>& axes) 
    {
      return { reference, axes };
    }

  //! \copydoc __assign_pos_axes
  template <class ImageType, typename IntType>
    inline __assign_pos_axes<ImageType, IntType> 
    assign_pos_of (const ImageType& reference, const std::vector<IntType>&& axes) 
    {
      return assign_pos_of (reference, axes);
    }



  //! returns the number of voxel in the data set, or a relevant subvolume
  template <class HeaderType> 
    inline size_t voxel_count (const HeaderType& in, size_t from_axis = 0, size_t to_axis = std::numeric_limits<size_t>::max())
    {
      if (to_axis > in.ndim()) to_axis = in.ndim();
      assert (from_axis < to_axis);
      size_t fp = 1;
      for (size_t n = from_axis; n < to_axis; ++n)
        fp *= in.size (n);
      return fp;
    }

  //! returns the number of voxel in the relevant subvolume of the data set
  template <class HeaderType> 
    inline size_t voxel_count (const HeaderType& in, const char* specifier)
    {
      size_t fp = 1;
      for (size_t n = 0; n < in.ndim(); ++n)
        if (specifier[n] != ' ') fp *= in.size (n);
      return fp;
    }

  //! returns the number of voxel in the relevant subvolume of the data set
  template <class HeaderType> 
    inline int64_t voxel_count (const HeaderType& in, const std::vector<size_t>& axes)
    {
      int64_t fp = 1;
      for (size_t n = 0; n < axes.size(); ++n) {
        assert (axes[n] < in.ndim());
        fp *= in.size (axes[n]);
      }
      return fp;
    }

  template <typename ValueType> 
    inline int64_t footprint (int64_t count) {
      return count * sizeof(ValueType);
    }

  template <> 
    inline int64_t footprint<bool> (int64_t count) {
      return (count+7)/8;
    }

  inline int64_t footprint (int64_t count, DataType dtype) {
    return dtype == DataType::Bit ? (count+7)/8 : count*dtype.bytes();
  }

  //! returns the memory footprint of an Image
  template <class HeaderType> 
    inline typename std::enable_if<std::is_class<HeaderType>::value, int64_t>::type footprint (const HeaderType& in, size_t from_dim = 0, size_t up_to_dim = std::numeric_limits<size_t>::max()) {
      return footprint (voxel_count (in, from_dim, up_to_dim), in.datatype());
    }

  //! returns the memory footprint of an Image
  template <class HeaderType> 
    inline typename std::enable_if<std::is_class<HeaderType>::value, int64_t>::type footprint (const HeaderType& in, const char* specifier) {
      return footprint (voxel_count (in, specifier), in.datatype());
    }




  template <class InfoType1, class InfoType2> 
    inline bool dimensions_match (const InfoType1& in1, const InfoType2& in2)
    {
      if (in1.ndim() != in2.ndim()) return false;
      for (size_t n = 0; n < in1.ndim(); ++n)
        if (in1.size (n) != in2.size (n)) return false;
      return true;
    }

  template <class InfoType1, class InfoType2> 
    inline bool dimensions_match (const InfoType1& in1, const InfoType2& in2, size_t from_axis, size_t to_axis)
    {
      assert (from_axis < to_axis);
      if (to_axis > in1.ndim() || to_axis > in2.ndim()) return false;
      for (size_t n = from_axis; n < to_axis; ++n)
        if (in1.size (n) != in2.size (n)) return false;
      return true;
    }

  template <class InfoType1, class InfoType2> 
    inline bool dimensions_match (const InfoType1& in1, const InfoType2& in2, const std::vector<size_t>& axes)
    {
      for (size_t n = 0; n < axes.size(); ++n) {
        if (in1.ndim() <= axes[n] || in2.ndim() <= axes[n]) return false;
        if (in1.size (axes[n]) != in2.size (axes[n])) return false;
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



  template <class HeaderType>
    inline void squeeze_dim (HeaderType& in, size_t from_axis = 3) 
    {
      size_t n = in.ndim();
      while (in.size(n-1) <= 1 && n > from_axis) --n;
      in.set_ndim (n);
    }



  namespace Helper
  {

    template <class ImageType>
      class Index {
        public:
          Index (ImageType& image, size_t axis) : image (image), axis (axis) { assert (axis < image.ndim()); }
          Index () = delete;
          Index (const Index&) = delete;
          Index (Index&&) = default;
          Index& operator= (Index&&) = delete;

          ssize_t get () const { const ImageType& _i (image); return _i.index (axis); }
          operator ssize_t () const { return get (); }
          ssize_t operator++ () { move( 1); return get(); }
          ssize_t operator-- () { move(-1); return get(); }
          ssize_t operator++ (int) { auto p = get(); move( 1); return p; }
          ssize_t operator-- (int) { auto p = get(); move(-1); return p; }
          ssize_t operator+= (ssize_t increment) { move( increment); return get(); }
          ssize_t operator-= (ssize_t increment) { move(-increment); return get(); }
          ssize_t operator= (ssize_t position) { return ( *this += position - get() ); }
          ssize_t operator= (const Index& position) { return ( *this = position.get() ); }
          friend std::ostream& operator<< (std::ostream& stream, const Index& p) { stream << p.get(); return stream; }
        protected:
          ImageType& image;
          const size_t axis;
          void move (ssize_t amount) { image.move_index (axis, amount); }
      };


    template <class ImageType> 
      class Value {
        public:
          typedef typename ImageType::value_type value_type;
          Value () = delete;
          Value (const Value&) = delete;
          Value (Value&&) = default;
          Value& operator= (Value&&) = delete;

          Value (ImageType& parent) : image (parent) { }
          value_type get () const { const ImageType& _i (image); return _i.value(); }
          operator value_type () const { return get(); }
          value_type operator= (value_type value) { return set (value); }
          value_type operator= (const Value& V) { return set (V.get()); }
          value_type operator+= (value_type value) { return set (get() + value); }
          value_type operator-= (value_type value) { return set (get() - value); }
          value_type operator*= (value_type value) { return set (get() * value); }
          value_type operator/= (value_type value) { return set (get() / value); }
          friend std::ostream& operator<< (std::ostream& stream, const Value& V) { stream << V.get(); return stream; }
      private:
        ImageType& image;
        value_type set (value_type value) { image.set_value (value); return value; }
    };



    //! convenience function returning fully-formed VoxelIndex 
    template <class ImageType>
      inline Index<ImageType> index (ImageType& image, size_t axis) {
        return { image, axis };
      }

    //! convenience function returning fully-formed VoxelValue
    template <class ImageType>
      inline Value<ImageType> value (ImageType& image) {
        return { image };
      }


  }
}

#endif





