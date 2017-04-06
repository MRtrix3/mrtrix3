/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#ifndef __image_helpers_h__
#define __image_helpers_h__

#include "datatype.h"
#include "apply.h"

namespace MR
{


  //! \cond skip
  namespace {

    template <class AxesType>
      FORCE_INLINE auto __ndim (const AxesType& axes) -> decltype (axes.size(), size_t()) { return axes.size(); }

    template <class AxesType>
      FORCE_INLINE auto __ndim (const AxesType& axes) -> decltype (axes.ndim(), size_t()) { return axes.ndim(); }

    template <class AxesType>
      FORCE_INLINE auto __get_index (const AxesType& axes, size_t axis) -> decltype (axes.size(), ssize_t()) 
      { return axes[axis]; }

    template <class AxesType>
      FORCE_INLINE auto __get_index (const AxesType& axes, size_t axis) -> decltype (axes.ndim(), ssize_t()) 
      { return axes.index(axis); }

    template <class AxesType>
      FORCE_INLINE auto __set_index (AxesType& axes, size_t axis, ssize_t index) -> decltype (axes.size(), void()) 
      { axes[axis] = index; }

    template <class AxesType>
      FORCE_INLINE auto __set_index (AxesType& axes, size_t axis, ssize_t index) -> decltype (axes.ndim(), void()) 
      { axes.index(axis) = index; }


    template <class... DestImageType>
      struct __assign { NOMEMALIGN
        __assign (size_t axis, ssize_t index) : axis (axis), index (index) { }
        const size_t axis;
        const ssize_t index;
        template <class ImageType> 
          FORCE_INLINE void operator() (ImageType& x) { __set_index (x, axis, index); }
      };

    template <class... DestImageType>
      struct __assign<std::tuple<DestImageType...>> { NOMEMALIGN
        __assign (size_t axis, ssize_t index) : axis (axis), index (index) { }
        const size_t axis;
        const ssize_t index;
        template <class ImageType> 
          FORCE_INLINE void operator() (ImageType& x) { apply (__assign<DestImageType...> (axis, index), x); }
      };

    template <class... DestImageType>
      struct __max_axis { NOMEMALIGN
        __max_axis (size_t& axis) : axis (axis) { }
        size_t& axis;
        template <class ImageType> 
          FORCE_INLINE void operator() (ImageType& x) { if (axis > __ndim(x)) axis = __ndim(x); }
      };

    template <class... DestImageType>
      struct __max_axis<std::tuple<DestImageType...>> { NOMEMALIGN
        __max_axis (size_t& axis) : axis (axis) { }
        size_t& axis;
        template <class ImageType> 
          FORCE_INLINE void operator() (ImageType& x) { apply (__max_axis<DestImageType...> (axis), x); }
      };

    template <class ImageType>
      struct __assign_pos_axis_range { NOMEMALIGN
        template <class... DestImageType>
          FORCE_INLINE void to (DestImageType&... dest) const {
            size_t last_axis = to_axis;
            apply (__max_axis<DestImageType...> (last_axis), std::tie (ref, dest...));
            for (size_t n = from_axis; n < last_axis; ++n)
              apply (__assign<DestImageType...> (n, __get_index (ref, n)), std::tie (dest...));
          }
        const ImageType& ref;
        const size_t from_axis, to_axis;
      };


    template <class ImageType, typename IntType>
      struct __assign_pos_axes { NOMEMALIGN
        template <class... DestImageType>
          FORCE_INLINE void to (DestImageType&... dest) const {
            for (auto a : axes) 
              apply (__assign<DestImageType...> (a, __get_index (ref, a)), std::tie (dest...));
          }
        const ImageType& ref;
        const vector<IntType> axes;
      };
  }

  template <typename ValueType> class Image;
  //! \endcond


  //! convenience function for SFINAE on header types
  template <class HeaderType, typename ReturnType>
    struct enable_if_header_type { NOMEMALIGN
      typedef decltype ((void) (
            std::declval<HeaderType>().ndim() + 
            std::declval<HeaderType>().size(0) + 
            std::declval<HeaderType>().name().size() 
        ), std::declval<ReturnType>()) type;
    };

  //! convenience function for SFINAE on header types
  template<typename HeaderType>
    class is_header_type { NOMEMALIGN
      typedef char yes[1], no[2];
      template<typename C> static yes& test(typename enable_if_header_type<HeaderType,int>::type); 
      template<typename C> static no&  test(...); 
      public:
      static bool const value = sizeof(test<HeaderType>(0)) == sizeof(yes);
    };




  //! convenience function for SFINAE on image types
  template <class ImageType, typename ReturnType>
    struct enable_if_image_type { NOMEMALIGN
      typedef decltype ((void) (
            std::declval<ImageType>().ndim() + 
            std::declval<ImageType>().size(0) + 
            std::declval<ImageType>().name().size() +
            std::declval<ImageType>().value() +
            std::declval<ImageType>().index(0)
        ), std::declval<ReturnType>()) type;
    };


  //! convenience function for SFINAE on image types
  template<typename ImageType>
    class is_image_type { NOMEMALIGN
      typedef char yes[1], no[2];
      template<typename C> static yes& test(typename enable_if_image_type<ImageType,int>::type); 
      template<typename C> static no&  test(...); 
      public:
      static bool const value = sizeof(test<ImageType>(0)) == sizeof(yes);
    };






  //! convenience function for SFINAE on images of type Image<ValueType>
  template<class ImageType>
    struct is_pure_image { NOMEMALIGN
      static bool const value = std::is_same<ImageType, ::MR::Image<typename ImageType::value_type>>::value;
    };

  //! convenience function for SFINAE on images NOT of type Image<ValueType>
  template<class ImageType>
    struct is_adapter_type { NOMEMALIGN
      static bool const value = is_image_type<ImageType>::value && !is_pure_image<ImageType>::value;
    };




  //! returns a functor to set the position in ref to other voxels
  /*! this can be used as follows:
   * \code
   * assign_pos_of (src_image, 0, 3).to (dest_image1, dest_image2);
   * \endcode 
   * 
   * This function will accept both ImageType objects (i.e. with ndim() &
   * index(size_t) methods) or VectorType objects (i.e. with size() &
   * operator[](size_t) methods). */
  template <class ImageType>
    FORCE_INLINE __assign_pos_axis_range<ImageType> 
    assign_pos_of (const ImageType& reference, size_t from_axis = 0, size_t to_axis = std::numeric_limits<size_t>::max()) 
    {
      return { reference, from_axis, to_axis };
    }

  //! returns a functor to set the position in ref to other voxels
  /*! this can be used as follows:
   * \code
   * vector<int> axes = { 0, 3, 4 };
   * assign_pos (src_image, axes) (dest_image1, dest_image2);
   * \endcode 
   * 
   * This function will accept both ImageType objects (i.e. with ndim() &
   * index(size_t) methods) or VectorType objects (i.e. with size() &
   * operator[](size_t) methods). */
  template <class ImageType, typename IntType>
    FORCE_INLINE __assign_pos_axes<ImageType, IntType> 
    assign_pos_of (const ImageType& reference, const vector<IntType>& axes) 
    {
      return { reference, axes };
    }

  template <class ImageType, typename IntType>
    FORCE_INLINE __assign_pos_axes<ImageType, IntType> 
    assign_pos_of (const ImageType& reference, const vector<IntType>&& axes) 
    {
      return assign_pos_of (reference, axes);
    }



  template <class ImageType>
    FORCE_INLINE bool is_out_of_bounds (const ImageType& image, 
        size_t from_axis = 0, size_t to_axis = std::numeric_limits<size_t>::max())
    {
      for (size_t n = from_axis; n < std::min<size_t> (to_axis, image.ndim()); ++n)
        if (image.index(n) < 0 || image.index(n) >= image.size(n))
          return true;
      return false;
    }

  template <class HeaderType, class VectorType>
    FORCE_INLINE typename std::enable_if<!std::is_arithmetic<VectorType>::value, bool>::type is_out_of_bounds (const HeaderType& header, const VectorType& pos,
        size_t from_axis = 0, size_t to_axis = std::numeric_limits<size_t>::max())
    {
      for (size_t n = from_axis; n < std::min<size_t> (to_axis, header.ndim()); ++n)
        if (pos[n] < 0 || pos[n] >= header.size(n))
          return true;
      return false;
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
    inline size_t voxel_count (const HeaderType& in, const std::initializer_list<size_t> axes)
    {
      size_t fp = 1;
      for (auto n : axes)
        fp *= in.size (n);
      return fp;
    }

  //! returns the number of voxel in the relevant subvolume of the data set
  template <class HeaderType> 
    inline int64_t voxel_count (const HeaderType& in, const vector<size_t>& axes)
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



  template <class HeaderType1, class HeaderType2>
    inline bool spacings_match (const HeaderType1& in1, const HeaderType2& in2)
    {
      if (in1.ndim() != in2.ndim()) return false;
      for (size_t n = 0; n < in1.ndim(); ++n)
        if (in1.spacing (n) != in2.spacing(n)) return false;
      return true;
    }

  template <class HeaderType1, class HeaderType2>
    inline bool spacings_match (const HeaderType1& in1, const HeaderType2& in2, size_t from_axis, size_t to_axis)
    {
      assert (from_axis < to_axis);
      if (to_axis > in1.ndim() || to_axis > in2.ndim()) return false;
      for (size_t n = from_axis; n < to_axis; ++n)
        if (in1.spacing (n) != in2.spacing (n)) return false;
      return true;
    }

  template <class HeaderType1, class HeaderType2>
    inline bool spacings_match (const HeaderType1& in1, const HeaderType2& in2, const vector<size_t>& axes)
    {
      for (size_t n = 0; n < axes.size(); ++n) {
        if (in1.ndim() <= axes[n] || in2.ndim() <= axes[n]) return false;
        if (in1.spacing (axes[n]) != in2.spacing (axes[n])) return false;
      }
      return true;
    }



  template <class HeaderType1, class HeaderType2>
    inline bool dimensions_match (const HeaderType1& in1, const HeaderType2& in2)
    {
      if (in1.ndim() != in2.ndim()) return false;
      for (size_t n = 0; n < in1.ndim(); ++n)
        if (in1.size (n) != in2.size (n)) return false;
      return true;
    }

  template <class HeaderType1, class HeaderType2>
    inline bool dimensions_match (const HeaderType1& in1, const HeaderType2& in2, size_t from_axis, size_t to_axis)
    {
      assert (from_axis < to_axis);
      if (to_axis > in1.ndim() || to_axis > in2.ndim()) return false;
      for (size_t n = from_axis; n < to_axis; ++n)
        if (in1.size (n) != in2.size (n)) return false;
      return true;
    }

  template <class HeaderType1, class HeaderType2>
    inline bool dimensions_match (const HeaderType1& in1, const HeaderType2& in2, const vector<size_t>& axes)
    {
      for (size_t n = 0; n < axes.size(); ++n) {
        if (in1.ndim() <= axes[n] || in2.ndim() <= axes[n]) return false;
        if (in1.size (axes[n]) != in2.size (axes[n])) return false;
      }
      return true;
    }

  template <class HeaderType1, class HeaderType2>
    inline void check_dimensions (const HeaderType1& in1, const HeaderType2& in2)
    {
      if (!dimensions_match (in1, in2))
        throw Exception ("dimension mismatch between \"" + in1.name() + "\" and \"" + in2.name() + "\"");
    }

  template <class HeaderType1, class HeaderType2>
    inline void check_dimensions (const HeaderType1& in1, const HeaderType2& in2, size_t from_axis, size_t to_axis)
    {
      if (!dimensions_match (in1, in2, from_axis, to_axis))
        throw Exception ("dimension mismatch between \"" + in1.name() + "\" and \"" + in2.name() + "\"");
    }

  template <class HeaderType1, class HeaderType2>
    inline void check_dimensions (const HeaderType1& in1, const HeaderType2& in2, const vector<size_t>& axes)
    {
      if (!dimensions_match (in1, in2, axes))
        throw Exception ("dimension mismatch between \"" + in1.name() + "\" and \"" + in2.name() + "\"");
    }

  template <class HeaderType1, class HeaderType2>
    inline void check_transform (const HeaderType1& in1, const HeaderType2& in2, const double tol = 0.0) {
      for (size_t i  = 0; i < 3; ++i) {
        for (size_t j  = 0; j < 4; ++j) {
          if (std::abs (in1.transform().matrix()(i,j) - in2.transform().matrix()(i,j)) > tol)
            throw Exception ("images \"" + in1.name() + "\" and \"" + in2.name() + "\" do not have matching header transforms "
                               + "\n" + str(in1.transform().matrix()) + "vs \n " + str(in2.transform().matrix()) + ")");
        }
      }
    }


  template <class HeaderType1, class HeaderType2>
    inline bool transforms_match (const HeaderType1 in1, const HeaderType2 in2, const double tol = 0.0) {
      for (size_t i  = 0; i < 3; ++i) {
        for (size_t j  = 0; j < 4; ++j) {
          if (std::abs (in1.transform().matrix()(i,j) - in2.transform().matrix()(i,j)) > tol)
            return false;
        }
      }
      return true;
    }

  template <class HeaderType>
    inline void squeeze_dim (HeaderType& in, size_t from_axis = 3) 
    {
      size_t n = in.ndim();
      while (in.size(n-1) <= 1 && n > from_axis) --n;
      in.ndim() = n;
    }



  namespace Helper
  {

    template <class ImageType>
      class Index { NOMEMALIGN
        public:
          FORCE_INLINE Index (ImageType& image, size_t axis) : image (image), axis (axis) { assert (axis < image.ndim()); }
          Index () = delete;
          Index (const Index&) = delete;
          FORCE_INLINE Index (Index&&) = default;

          FORCE_INLINE operator ssize_t () const { return get (); }
          FORCE_INLINE ssize_t operator++ () { move( 1); return get(); }
          FORCE_INLINE ssize_t operator-- () { move(-1); return get(); }
          FORCE_INLINE ssize_t operator++ (int) { auto p = get(); move( 1); return p; }
          FORCE_INLINE ssize_t operator-- (int) { auto p = get(); move(-1); return p; }
          FORCE_INLINE ssize_t operator+= (ssize_t increment) { move( increment); return get(); }
          FORCE_INLINE ssize_t operator-= (ssize_t increment) { move(-increment); return get(); }
          FORCE_INLINE ssize_t operator= (ssize_t position) { return ( *this += position - get() ); }
          FORCE_INLINE ssize_t operator= (Index&& position) { return ( *this = position.get() ); }
          friend std::ostream& operator<< (std::ostream& stream, const Index& p) { stream << p.get(); return stream; }
        protected:
          ImageType& image;
          const size_t axis;
          FORCE_INLINE ssize_t get () const { return image.get_index (axis); }
          FORCE_INLINE void move (ssize_t amount) { image.move_index (axis, amount); }
      };


    template <class ImageType> 
      class Value { NOMEMALIGN
        public:
          using value_type = typename ImageType::value_type;
          Value () = delete;
          Value (const Value&) = delete;
          FORCE_INLINE Value (Value&&) = default;

          FORCE_INLINE Value (ImageType& parent) : image (parent) { }
          FORCE_INLINE operator value_type () const { return get(); }
          FORCE_INLINE value_type operator= (value_type value) { return set (value); }
          template <typename OtherType>
            FORCE_INLINE value_type operator= (Value<OtherType>&& V) { return set (typename OtherType::value_type (V)); }
          FORCE_INLINE value_type operator+= (value_type value) { return set (get() + value); }
          FORCE_INLINE value_type operator-= (value_type value) { return set (get() - value); }
          FORCE_INLINE value_type operator*= (value_type value) { return set (get() * value); }
          FORCE_INLINE value_type operator/= (value_type value) { return set (get() / value); }
          friend std::ostream& operator<< (std::ostream& stream, const Value& V) { stream << V.get(); return stream; }
      private:
        ImageType& image;
          FORCE_INLINE value_type get () const { return image.get_value(); }
        FORCE_INLINE value_type set (value_type value) { image.set_value (value); return value; }
    };


    template <class ImageType>
      class ConstRow { NOMEMALIGN
        public:
          ConstRow (ImageType& image, size_t axis) : axis (axis), image (image) { assert (axis >= 0 && axis < image.ndim()); }
          ssize_t size () const { return image.size (axis); }
          typename ImageType::value_type operator[] (ssize_t n) const { image.index (axis) = n; return image.value(); }
          const size_t axis;
        protected:
          ImageType& image;
          template <typename, int, int, int, int, int> friend class Eigen::Matrix;
          template <class Derived>        friend class Eigen::MatrixBase;
          template <class OtherImageType> friend class Row;
      };


    template <class ImageType>
      class Row :
        public ConstRow<ImageType> 
    { NOMEMALIGN
      public:

        typedef typename ImageType::value_type value_type;

        Row (ImageType& image, size_t axis) : ConstRow<ImageType> (image, axis) { }

        template <class OtherImageType>
          Row (ConstRow<OtherImageType>&& other) {
            assert (image.size(axis) == other.image.size(other.axis)); 
            for (image.index(axis) = 0, other.image.index(other.axis); 
                image.index(axis) < image.size(axis);
                ++image.index(axis), ++other.image.index(other.axis))  
              image.value() = typename OtherImageType::value_type (other.image.value()); 
          }

        using ConstRow<ImageType>::image;
        using ConstRow<ImageType>::axis;

#define MRTRIX_OP(ARG) \
        template <class Derived> \
          FORCE_INLINE void operator ARG (const Eigen::MatrixBase<Derived>& vec) { \
            assert (vec.rows() == image.size(axis)); \
            assert (vec.cols() == 1); \
            for (image.index(axis) = 0; image.index(axis) < image.size(axis); ++image.index(axis))  \
              image.value() ARG vec[image.index(axis)]; \
          }
        MRTRIX_OP(=);
        MRTRIX_OP(+=);
        MRTRIX_OP(-=);
#undef MRTRIX_OP

#define MRTRIX_OP(ARG) \
        FORCE_INLINE void operator ARG (value_type val) { \
          for (image.index(axis) = 0; image.index(axis) < image.size(axis); ++image.index(axis))  \
            image.value() ARG val; \
        }
        MRTRIX_OP(=);
        MRTRIX_OP(+=);
        MRTRIX_OP(-=);
        MRTRIX_OP(*=);
        MRTRIX_OP(/=);
#undef MRTRIX_OP

        FORCE_INLINE void operator= (Row&& other) { 
          assert (image.size(axis) == other.image.size(other.axis));  
            for (image.index(axis) = 0, other.image.index(other.axis) = 0;  
                image.index(axis) < image.size(axis); 
                ++image.index(axis), ++other.image.index(other.axis))   
              image.value() = other.image.value();  
        }

#define MRTRIX_OP(ARG) \
                template <class OtherImageType> \
          FORCE_INLINE void operator ARG (ConstRow<OtherImageType>&& other) { \
            assert (image.size(axis) == other.image.size(other.axis));  \
            for (image.index(axis) = 0, other.image.index(other.axis) = 0;  \
                image.index(axis) < image.size(axis); \
                ++image.index(axis), ++other.image.index(other.axis))   \
              image.value() ARG typename OtherImageType::value_type (other.image.value());  \
          }
        MRTRIX_OP(=);
        MRTRIX_OP(+=);
        MRTRIX_OP(-=);
#undef MRTRIX_OP

    };



  }


  template <class Derived, typename ValueType>
    class ImageBase 
    { MEMALIGN (ImageBase<Derived,ValueType>)
      public:
        typedef ValueType value_type;

        FORCE_INLINE Helper::Index<Derived> index (size_t axis) { return { static_cast<Derived&> (*this), axis }; }
        FORCE_INLINE ssize_t index (size_t axis) const { return static_cast<const Derived*>(this)->get_index (axis); }

        FORCE_INLINE Helper::Value<Derived> value () { return { static_cast<Derived&> (*this) }; }
        FORCE_INLINE ValueType value () const { return static_cast<const Derived*>(this)->get_value(); }

        //! a proxy class for the vector of values along the specified axis
        /*! returns a proxy class to simplify interactions with the data as a
         * vector along the specified axis. This class can be cast to an Eigen
         * matrix type, and can be assigned to using another instance of the
         * same class, or using an Eigen type. For example:
         * \code
         * Image<float> in; // assuming size 3 along volume dimension
         *
         * in.row(3) = Eigen::Vector3f::Random(3,1);
         * Eigen::Vector3f x (in.row(3));
         * out.row(3) += x;
         * out.row(3) += M*Eigen::Vector3f(in.row(3)) + Eigen::VectorXf (other.row(3));
         * \code
         * */
        FORCE_INLINE Helper::ConstRow<Derived> row (size_t axis) const { return { static_cast<Derived&> (*this), axis }; }
        FORCE_INLINE Helper::Row<Derived> row (size_t axis) { return { static_cast<Derived&> (*this), axis }; }
    };



}

#endif





