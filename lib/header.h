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

#ifndef __header_h__
#define __header_h__

#include <map>

#include "debug.h"
#include "types.h"
#include "memory.h"
#include "datatype.h"
#include "stride.h"
#include "file/mmap.h"
#include "math/matrix.h"

namespace MR
{
  /*! \defgroup ImageAPI Image access
   * \brief Classes and functions providing access to image data. */
  // @{

  //! functions and classes related to image data input/output


  namespace ImageIO { class Base; }
  template <typename ValueType> class Image;

  class Header 
  {
    public:
      class Axis;

      Header (Header&&) = default;

      //! copy constructor
      /*! This copies everything over apart from the IO handler and the
       * intensity scaling. */
      Header (const Header& H) :
        axes_ (H.axes_),
        transform_ (H.transform_),
        name_ (H.name_),
        keyval_ (H.keyval_),
        datatype_ (H.datatype_),
        offset_ (0.0),
        scale_ (1.0) { }

      Header& operator= (const Header& H) {
        axes_ = H.axes_;
        transform_ = H.transform_;
        name_ = H.name_;
        keyval_ = H.keyval_;
        datatype_ = H.datatype_;
        offset_ = 0.0;
        scale_ = 1.0;
        io.reset();
        return *this;
      }

      //! get the name of the image
      const std::string& name () const { return name_; }
      //! get/set the name of the image
      std::string& name () { return name_; }

      //! get the 4x4 affine transformation matrix mapping image to world coordinates
      const Math::Matrix<default_type>& transform () const { return transform_; }
      //! get/set the 4x4 affine transformation matrix mapping image to world coordinates
      Math::Matrix<default_type>& transform () { return transform_; }

      //! return the number of dimensions (axes) of image
      size_t ndim () const;
      //! set the number of dimensions (axes) of image
      void set_ndim (size_t new_ndim);

      //! get the number of voxels across axis
      const ssize_t& size (size_t axis) const;
      //! get/set the number of voxels across axis
      ssize_t& size (size_t axis);

      //! get the voxel size along axis
      const default_type& voxsize (size_t axis) const;
      //! get/set the voxel size along axis
      default_type& voxsize (size_t axis);

      //! get the stride between adjacent voxels along axis
      const ssize_t& stride (size_t axis) const;
      //! get/set the stride between adjacent voxels along axis
      ssize_t& stride (size_t axis);

      //! get the datatype of the data as stored on file
      const DataType datatype () const { return datatype_; }
      //! get/set the datatype of the data as stored on file
      DataType datatype () { return datatype_; }

      //! get the offset applied to raw intensities
      float intensity_offset () const { return offset_; }
      //! get/set the offset applied to raw intensities
      float& intensity_offset () { return offset_; }
      //! get the scaling applied to raw intensities
      float intensity_scale () const { return scale_; }
      //! get/set the scaling applied to raw intensities
      float& intensity_scale () { return scale_; }

      //! update existing intensity offset & scale with values supplied
      void apply_intensity_scaling (float scaling, float bias = 0.0) {
        scale_ *= scaling;
        offset_ = scaling * offset_ + bias;
      }
      //! replace existing intensity offset & scale with values supplied
      void set_intensity_scaling (float scaling = 1.0, float bias = 0.0) {
        scale_ = scaling;
        offset_ = bias;
      }
      //! replace existing intensity offset & scale with values in \a H
      void set_intensity_scaling (const Header& H) { set_intensity_scaling (H.intensity_scale(), H.intensity_offset()); }
      //! reset intensity offset to zero and scaling to one
      void reset_intensity_scaling () { set_intensity_scaling (); }

      bool is_file_backed () const { return bool (io); }

      //! make header self-consistent
      void sanitise () {
        DEBUG ("sanitising image information...");
        sanitise_voxel_sizes ();
        sanitise_transform ();
        sanitise_strides ();
      }

      //! return an Image to access the data
      /*! when this method is invoked, the image data will actually be made
       * available (i.e. it will be mapped, loaded or allocated into memory).
       *
       * \note this function should only be invoked on const objects - you should
       * not try to use it on a non-const Header object, it will result in a
       * compile-time error - this it to ensure that the Header has not been
       * modified since being created. */
      template <typename ValueType>
        Image<ValueType> get_image () const;

      //! get generic key/value text attributes
      const std::map<std::string, std::string>& keyval () const { return keyval_; }
      //! get/set generic key/value text attributes
      std::map<std::string, std::string>& keyval () { return keyval_; }

      template <typename ValueType=default_type> 
        Math::Matrix<ValueType> parse_DW_scheme () const
        {
          Math::Matrix<ValueType> G;
          const auto it = keyval().find ("dw_scheme");
          if (it != keyval().end()) {
            const auto lines = split_lines (it->second);
            for (size_t row = 0; row < lines.size(); ++row) {
              const auto values = parse_floats (lines[row]);
              if (G.columns() == 0)
                G.allocate (lines.size(), values.size());
              else if (G.columns() != values.size())
                throw Exception ("malformed DW scheme in image \"" + name() + "\" - uneven number of entries per row");
              for (size_t col = 0; col < values.size(); ++col)
                G(row, col) = values[col];
            }
          }
          return G;
        }

      template <typename ValueType> 
        void set_DW_scheme (const Math::Matrix<ValueType>& G)
        {
          std::string dw_scheme;
          for (size_t row = 0; row < G.rows(); ++row) {
            std::string line;
            for (size_t col = 0; col < G.columns(); ++col) {
              if (col == 0) line += ",";
              line += str(G(row,col));
            }
            add_line (dw_scheme, line);
          }
          keyval()["dw_scheme"] = dw_scheme;
        }

      static const Header open (const std::string& image_name);
      static const Header create (const std::string& image_name);
      static const Header allocate (const Header& template_header);
      static Header empty ();

      /*! use to prevent automatic realignment of transform matrix into
       * near-standard (RAS) coordinate system. */
      static bool do_not_realign_transform;

      //! return a string with the full description of the header
      std::string description() const;
      //! print out debugging information 
      friend std::ostream& operator<< (std::ostream& stream, const Header& H)
      {
        stream << H.description();
        return stream;
      }



      //! the offset to the voxel at the origin (used internally)
      // TODO remove?      size_t __data_start;

    protected:
      std::vector<Axis> axes_; 
      Math::Matrix<default_type> transform_; 
      std::string name_;
      std::map<std::string, std::string> keyval_;

      //! additional information relevant for images stored on file
      mutable std::unique_ptr<ImageIO::Base> io; 
      //! the type of the data as stored on file
      DataType datatype_; 
      //! the values by which to scale the intensities
      default_type offset_, scale_;


      Header () = default; // static methods should be used to create Header

      void acquire_io (const Header& H) { io = std::move (H.io); }
      void merge (const Header& H);

      //! realign transform to match RAS coordinate system as closely as possible
      void realign_transform ();

      void sanitise_voxel_sizes ();
      void sanitise_transform ();
      void sanitise_strides () {
        Stride::sanitise (*this);
        Stride::actualise (*this);
      }

      template <typename ValueType>
        Image<ValueType> get_image (); // do not use this function with a non-const Header
  };






  //! a class to hold attributes about each axis
  class Header::Axis {
    public:
      Axis () : size (1), voxsize (std::numeric_limits<default_type>::quiet_NaN()), stride (0) { }
      ssize_t size;
      default_type voxsize;
      ssize_t stride;
  };







  inline size_t Header::ndim () const { return axes_.size(); }
  inline void Header::set_ndim (size_t new_ndim) { axes_.resize (new_ndim); }

  inline const ssize_t& Header::size (size_t axis) const { return axes_[axis].size; }
  inline ssize_t& Header::size (size_t axis) { return axes_[axis].size; }

  inline const default_type& Header::voxsize (size_t axis) const { return axes_[axis].voxsize; }
  inline default_type& Header::voxsize (size_t axis) { return axes_[axis].voxsize; }

  inline const ssize_t& Header::stride (size_t axis) const { return axes_[axis].stride; }
  inline ssize_t& Header::stride (size_t axis) { return axes_[axis].stride; } 











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




  //! return whether the HeaderType contains complex data
  template <class HeaderType> 
    inline bool is_complex (const HeaderType&)
    {
      typedef typename HeaderType::default_type T;
      return is_complex__<T> ();
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




  //! @}
}

#include "image_io/base.h" // std::unique_ptr requires knowledge of sizeof (ImageIO::Base) 

#endif

