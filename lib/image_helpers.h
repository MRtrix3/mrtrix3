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

  inline int64_t footprint (int64_t count, DataType dtype) {
    return dtype == DataType::Bit ? (count+7)/8 : count*dtype.bytes();
  }

  //! returns the memory footprint of an Image
  template <class HeaderType> 
    inline int64_t footprint (const HeaderType& in, size_t from_dim = 0, size_t up_to_dim = std::numeric_limits<size_t>::max()) {
      return footprint (voxel_count (in, from_dim, up_to_dim), in.datatype());
    }

  //! returns the memory footprint of an Image
  template <class HeaderType> 
    inline int64_t footprint (const HeaderType& in, const char* specifier) {
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

    /*! \brief a class to simplify the implementation of ImageType classes with
     * non-trivial access to their position.
     *
     * This class provides a means of returning a modifiable voxel position
     * from a class, when the process of modifying the position cannot be done
     * by simply returning a non-const reference. This is best illustrated with
     * an example.
     *
     * Consider a ImageType class that keeps track of the offset to the voxel of
     * interest, by updating the offset every time the position along any axis
     * is modified (this is actually how most built-in ImageType instances
     * operate). This would require an additional operation to be performed
     * on top of the assignment itself. In other words,
     * \code
     * vox.index(1) = 12;
     * \endcode
     * cannot be done simply by returning a reference to the corresponding
     * position along axis 1, since the offset would be left unmodified, and
     * would therefore no longer point to the correct voxel. This can be solved
     * by returning a Helper::VoxelIndex instead, as illustated below.
     *
     * The following class implements the desired interface, but the offset
     * is not updated when required:
     * \code
     * class MyImage
     * {
     *   public:
     *     MyImage (ssize_t xdim, ssize_t ydim, ssize_t zdim) :
     *       offset (0)
     *     {
     *       // dataset dimensions:
     *       N[0] = xdim; N[1] = ydim; N[2] = zdim;
     *       // voxel positions along each axis:
     *       X[0] = X[1] = X[2] = 0;
     *       // strides to compute correct offset;
     *       S[0] = 1; S[1] = N[0]; S[2] = N[0]*N[1];
     *       // allocate data:
     *       data = new float [N[0]*N[1]*N[2]);
     *     }
     *     ~MyImage () { delete [] data; }
     *
     *     typedef float value_type;
     *
     *     size_t     ndim () const            { return 3; }
     *     ssize_t    dim (size_t axis) const  { return N[axis]; }
     *
     *     // PROBLEM: won't modify the offset when the position is modified!
     *     ssize_t&   index (size_t axis) { return X[axis]; }
     *
     *     float&  value () { return (data[offset]); }
     *
     *   private:
     *     float*  data
     *     ssize_t N[3], X[3], S[3], offset;
     * };
     * \endcode
     */
    /* The Helper::VoxelIndex class provides a solution to this problem. To use
     * it, the ImageType class must implement the get_voxel_position(),
     * set_voxel_position(), and move_voxel_position() methods. While these can
     * be declared public, it is probably cleaner to make them private or
     * protected, and to declare the Helper::VoxelIndex class as a friend.
     * 
     * \code
     * class MyImage
     * {
     *   public:
     *     MyImage (ssize_t xdim, ssize_t ydim, ssize_t zdim) :
     *       offset (0)
     *     {
     *       // dataset dimensions:
     *       N[0] = xdim; N[1] = ydim; N[2] = zdim;
     *       // voxel positions along each axis:
     *       X[0] = X[1] = X[2] = 0;
     *       // strides to compute correct offset;
     *       S[0] = 1; S[1] = N[0]; S[2] = N[0]*N[1];
     *       // allocate data:
     *       data = new float [N[0]*N[1]*N[2]);
     *     }
     *     ~MyImage () { delete [] data; }
     *
     *     typedef float value_type;
     *
     *     size_t     ndim () const            { return 3; }
     *     ssize_t    dim (size_t axis) const  { return N[axis]; }
     *
     *     // FIX: return a Helper::VoxelIndex<MyImage> class:
     *     Helper::VoxelIndex<MyImage> index (size_t axis) {
     *       return Helper::VoxelIndex<MyImage> (*this, axis);
     *     }
     *
     *     float&  value () { return data[offset]; }
     *
     *   private:
     *     float*  data
     *     ssize_t N[3], X[3], S[3], offset;
     *
     *     // this function returns the voxel position along the specified axis,
     *     // in a non-modifiable way:
     *     ssize_t get_voxel_position (size_t axis) const { return X[axis]; }
     *
     *     // this function sets the voxel position along the specified axis:
     *     void set_voxel_position (size_t axis, ssize_t index) const {
     *       offset += S[axis] * (index - X[axis]);
     *       X[axis] = index;
     *     }
     *
     *     // this function moves the voxel position by the specified increment
     *     // along the speficied axis:
     *     void move_voxel_position (size_t axis, ssize_t increment) {
     *       offset += S[axis] * increment;
     *       X[axis] += increment;
     *     }
     *
     *     friend class Helper::VoxelIndex<MyImage>;
     * };
     * \endcode */
    /* In the example above, a Helper::VoxelIndex instance is returned by the
     * index() function, and can be manipulated using standard operators. For
     * instance, the following code fragment is allowed, and will update the
     * offset each time as expected:
     *
     * \code
     * // create an instance of MyImage:
     * MyImage data (100, 100, 100);
     *
     * // setting the position of the voxel will cause
     * // the set_voxel_position() function to be invoked each time:
     * // ensuring the offset is up to date:
     * data.index(0) = 10;
     *
     * // setting the position also causes the get_voxel_position() function to
     * // be invoked, so that the new position can be returned.
     * // This allows chaining of the assignments, e.g.:
     * data.index(1) = data.index(2) = 20;
     *
     * // incrementing the position will cause the move_voxel_position()
     * // function to be invoked (and also the get_voxel_position() function
     * // so that the new position can be returned):
     * data.index(0) += 10;
     *
     * // reading the position will cause the get_voxel_position()
     * // function to be invoked:
     * ssize_t xpos = data.index(0);
     *
     * // this implies that the voxel position can be used
     * // in simple looping constructs:
     * float sum = 0.0
     * for (data.index(1) = 0; data.index(1) < data.dim(1); ++data.index(1))
     *   sum += data.value();
     * \endcode
     *
     * \section performance Performance
     * The use of this class or of the related Helper::VoxelValue class does not
     * impose any measurable performance penalty if the code is
     * compiled for release (i.e. with optimisations turned on and debugging
     * symbols turned off). This is the default setting for the configure
     * script, unless the -debug option is supplied.
     */
    template <class ImageType> class VoxelIndex
    {
      public:
        VoxelIndex (ImageType& parent, size_t corresponding_axis) : S (parent), axis (corresponding_axis) {
          assert (axis < S.ndim());
        }
        operator ssize_t () const {
          return S.get_voxel_position (axis);
        }
        ssize_t operator++ () {
          S.move_voxel_position (axis,1);
          return S.get_voxel_position (axis);
        }
        ssize_t operator-- () {
          S.move_voxel_position (axis,-1);
          return S.get_voxel_position (axis);
        }
        ssize_t operator++ (int) {
          const ssize_t p = S.get_voxel_position (axis);
          S.move_voxel_position (axis,1);
          return p;
        }
        ssize_t operator-- (int) {
          const ssize_t p = S.get_voxel_position (axis);
          S.move_voxel_position (axis,-1);
          return p;
        }
        ssize_t operator+= (ssize_t increment) {
          S.move_voxel_position (axis, increment);
          return S.get_voxel_position (axis);
        }
        ssize_t operator-= (ssize_t increment) {
          S.move_voxel_position (axis, -increment);
          return S.get_voxel_position (axis);
        }
        ssize_t operator= (ssize_t position) {
          S.set_voxel_position (axis, position);
          return position;
        }
        ssize_t operator= (const VoxelIndex& position) {
          S.set_voxel_position (axis, ssize_t (position));
          return ssize_t (position);
        }
        friend std::ostream& operator<< (std::ostream& stream, const VoxelIndex& p) {
          stream << ssize_t (p);
          return stream;
        }
      protected:
        ImageType& S;
        const size_t axis;
    };












    /*! \brief a class to simplify the implementation of ImageType classes with
     * non-trivial access to their data.
     *
     * This class provides a means of returning a modifiable voxel value from a
     * class, when the process of modifying a value cannot be done by simply
     * returning a non-const reference. This is best illustrated with an
     * example.
     *
     * Consider a ImageType class that perform a validity check on each voxel
     * value as it is stored. For example, it might enforce a policy that
     * values should be clamped to a particular range. Such a class might look
     * like the following:
     * \code
     * class MyImage
     * {
     *   public:
     *     MyImage (ssize_t xdim, ssize_t ydim, ssize_t zdim)
     *     {
     *       N[0] = xdim; N[1] = ydim; N[2] = zdim;
     *       X[0] = X[1] = X[2] = 0;
     *       data = new float [N[0]*N[1]*N[2]);
     *     }
     *     ~MyImage () { delete [] data; }
     *
     *     typedef float value_type;
     *
     *     size_t     ndim () const           { return (3); }
     *     ssize_t    dim (size_t axis) const    { return (N[axis]); }
     *     ssize_t&   operator[] (size_t axis)   { return (X[axis]); }
     *
     *     // PROBLEM: can't check that the value is valid when it is modified!
     *     float&  value () { return (data[X[0]+N[0]*(X[1]+N[1]*X[2])]); }
     *
     *   private:
     *     float*   data
     *     ssize_t  N[3];
     *     ssize_t  X[3];
     * };
     * \endcode
     */
    /* While it is possible in the above example to modify the voxel values,
     * since they are returned by reference, it is also impossible to implement
     * clamping of the voxel values as they are modified. The Helper::VoxelValue
     * class provides a solution to this problem.
     *
     * To use the Helper::VoxelValue interface, the ImageType class must implement a
     * get_voxel_value() and a set_voxel_value() method. While these can be declared
     * public, it is probably cleaner to make them private or protected, and to
     * declare the Helper::VoxelValue class as a friend.
     * \code
     * class MyImage
     * {
     *   public:
     *     MyImage (ssize_t xdim, ssize_t ydim, ssize_t zdim)
     *     {
     *       N[0] = xdim; N[1] = ydim; N[2] = zdim;
     *       X[0] = X[1] = X[2] = 0;
     *       data = new float [N[0]*N[1]*N[2]);
     *     }
     *     ~MyImage () { delete [] data; }
     *
     *     typedef float value_type;
     *
     *     size_t     ndim () const           { return (3); }
     *     ssize_t    dim (size_t axis) const    { return (N[axis]); }
     *     ssize_t&   operator[] (size_t axis)   { return (X[axis]); }
     *
     *     // FIX: return a Helper::VoxelValue<MyImage> class:
     *     Helper::VoxelValue<MyImage>  value () { return (Helper::VoxelValue<MyImage> (*this); }
     *
     *   private:
     *     float*   data
     *     ssize_t  N[3];
     *     ssize_t  X[3];
     *
     *     // this function returns the voxel value, in a non-modifiable way:
     *     value_type get_voxel_value () const { return (data[X[0]+N[0]*(X[1]+N[1]*X[2])]); }
     *
     *     // this function ensures that the value supplied is clamped
     *     // between 0 and 1 before updating the voxel value:
     *     void set_voxel_value (value_type val) {
     *       if (val < 0.0) val = 0.0;
     *       if (val > 1.0) val = 1.0;
     *       data[X[0]+N[0]*(X[1]+N[1]*X[2])] = val;
     *     }
     *
     *     friend class Helper::VoxelValue<MyImage>;
     * };
     * \endcode
     */
    /* In the example above, a Helper::VoxelValue instance is returned by the
     * value() function, and can be manipulated using standard operators. For
     * instance, the following code fragment is allowed, and will function as
     * expected with clamping of the voxel values when any voxel value is
     * modified:
     * \code
     * // create an instance of MyImage:
     * MyImage data (100, 100, 100);
     *
     * // set the position of the voxel:
     * data.index(0) = X;
     * data.index(1) = Y;
     * data.index(2) = Z;
     *
     * // set the voxel value, clamped to the range [0.0 1.0]:
     * data.value() = 2.3;
     *
     * // other operators can also be used, for example:
     * data.value() += 10.0;
     * data.value() /= 5.0;
     * \endcode
     *
     * \section performance Performance
     * The use of this class or of the related Helper::VoxelIndex class does not
     * impose any measurable performance penalty if the code is
     * compiled for release (i.e. with optimisations turned on and debugging
     * symbols turned off). This is the default setting for the configure
     * script, unless the -debug option is supplied.
     */
    template <class ImageType> class VoxelValue
    {
      public:
        typedef typename ImageType::value_type value_type;

        VoxelValue (ImageType& parent) : S (parent) { }
        operator value_type () const {
          return get_voxel_value();
        }
        value_type operator= (value_type value) {
          return set_value (value);
        }
        value_type operator= (const VoxelValue& V) {
          value_type value = V.get_voxel_value();
          return set_value (value);
        }
        value_type operator+= (value_type value) {
          value += get_voxel_value();
          return set_value (value);
        }
        value_type operator-= (value_type value) {
          value = get_voxel_value() - value;
          return set_value (value);
        }
        value_type operator*= (value_type value) {
          value *= get_voxel_value();
          return set_value (value);
        }
        value_type operator/= (value_type value) {
          value = get_voxel_value() / value;
          return set_value (value);
        }

        //! return RAM address of current voxel
        /*! \note this will only work with Image::BufferPreload and
         * Image::BufferScratch. */
        value_type* address () { 
          return S.address();
        }

        friend std::ostream& operator<< (std::ostream& stream, const VoxelValue& V) {
          stream << V.get_voxel_value();
          return stream;
        }
      private:
        ImageType& S;

        value_type get_voxel_value () const { 
          return S.get_voxel_value();
        }
        value_type set_value (value_type value) {
          S.set_voxel_value (value);
          return value;
        }
    };


    //! convenience function returning fully-formed VoxelIndex 
    template <class ImageType>
      VoxelIndex<ImageType> voxel_index (ImageType& image, size_t axis) {
        return { image, axis };
      }

    //! convenience function returning fully-formed VoxelValue
    template <class ImageType>
      VoxelValue<ImageType> voxel_value (ImageType& image) {
        return { image };
      }

  }
}

#endif





