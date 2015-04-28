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
#include "image_helpers.h"

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

      Header () :
        offset_ (0.0),
        scale_ (1.0) { }
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

      bool valid () const { return axes_.size() >= 3; }
      bool operator! () const { return !valid(); }

      //! get the name of the image
      const std::string& name () const { return name_; }
      //! get/set the name of the image
      std::string& name () { return name_; }

      //! return the format of the image, or null if not file-backed
      const char* format () const;

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
      DataType datatype () const { return datatype_; }
      //! get/set the datatype of the data as stored on file
      DataType& datatype () { return datatype_; }

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
        const Image<ValueType> get_image () const;

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
              if (!G.is_set())
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
      static const Header create (const std::string& image_name, const Header& template_header);
      static const Header allocate (const Header& template_header);
      static const Header empty ();

      /*! use to prevent automatic realignment of transform matrix into
       * near-standard (RAS) coordinate system. */
      static bool do_not_realign_transform;

      //! return a string with the full description of the header
      std::string description() const;
      //! print out debugging information 
      friend std::ostream& operator<< (std::ostream& stream, const Header& H);

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
        const Image<ValueType> get_image (); // do not use this function with a non-const Header
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


  inline const Header Header::allocate (const Header& template_header) 
  {
    Header H (template_header);
    H.sanitise();
    return H;
  }

  inline const Header Header::empty ()
  {
    return { };
  }





  //! @}
}

#include "image_io/base.h" // std::unique_ptr requires knowledge of sizeof (ImageIO::Base) 

#endif

