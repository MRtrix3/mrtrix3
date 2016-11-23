/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#ifndef __header_h__
#define __header_h__

#include <map>
#include <functional>

#include "debug.h"
#include "types.h"
#include "memory.h"
#include "datatype.h"
#include "stride.h"
#include "file/mmap.h"
#include "image_helpers.h"
#include "image_io/base.h" 


namespace MR
{

  /*! \defgroup ImageAPI Image access
   * \brief Classes and functions providing access to image data. 
   *
   * See @ref image_access for details. */
  // @{

  //! functions and classes related to image data input/output


  template <typename ValueType> class Image;

  class Header { MEMALIGN (Header)
    public:
      class Axis;

      Header () :
        transform_ (Eigen::Matrix<default_type,3,4>::Constant (NaN)),
        format_ (nullptr),
        offset_ (0.0),
        scale_ (1.0) {
        }

      explicit Header (Header&& H) noexcept :
        axes_ (std::move (H.axes_)),
        transform_ (std::move (H.transform_)),
        name_ (std::move (H.name_)),
        keyval_ (std::move (H.keyval_)),
        format_ (H.format_),
        io (std::move (H.io)),
        datatype_ (std::move (H.datatype_)),
        offset_ (H.offset_),
        scale_ (H.scale_) {}

      Header& operator= (Header&& H) noexcept {
        axes_ = std::move (H.axes_);
        transform_ = std::move (H.transform_);
        name_ = std::move (H.name_);
        keyval_ = std::move (H.keyval_);
        format_ = H.format_;
        io = std::move (H.io);
        datatype_ = std::move (H.datatype_);
        offset_ = H.offset_;
        scale_ = H.scale_;
        return *this;
      }

      //! copy constructor
      /*! This copies everything over apart from the IO handler, and resets the
       * intensity scaling if the datatype is floating-point. */
      Header (const Header& H) :
        axes_ (H.axes_),
        transform_ (H.transform_),
        name_ (H.name_),
        keyval_ (H.keyval_),
        format_ (H.format_),
        datatype_ (H.datatype_),
        offset_ (datatype().is_integer() ? H.offset_ : 0.0),
        scale_ (datatype().is_integer() ? H.scale_ : 1.0) { }

      //! copy constructor from type of class derived from Header
      /*! This invokes the standard Header(const Header&) copy-constructor. */
      template <class HeaderType, typename std::enable_if<std::is_base_of<Header, HeaderType>::value, void*>::type = nullptr>
        Header (const HeaderType& original) :
          Header (static_cast<const Header&> (original)) { }

      //! copy constructor from type of class other than Header
      /*! This copies all relevant parameters over from \a original. */ 
      template <class HeaderType, typename std::enable_if<!std::is_base_of<Header, HeaderType>::value, void*>::type = nullptr>
        Header (const HeaderType& original) :
          transform_ (original.transform()),
          name_ (original.name()),
          keyval_ (original.keyval()),
          format_ (nullptr),
          datatype_ (DataType::from<typename HeaderType::value_type>()),
          offset_ (0.0),
          scale_ (1.0) {
            axes_.resize (original.ndim());
            for (size_t n = 0; n < original.ndim(); ++n) {
              size(n) = original.size(n);
              stride(n) = original.stride(n);
              spacing(n) = original.spacing(n);
            }
          }


      //! assignment operator
      /*! This copies everything over, resets the intensity scaling if the data
       * type is floating-point, and resets the IO handler. */
      Header& operator= (const Header& H) {
        axes_ = H.axes_;
        transform_ = H.transform_;
        name_ = H.name_;
        keyval_ = H.keyval_;
        format_ = H.format_;
        datatype_ = H.datatype_;
        offset_ = datatype().is_integer() ? H.offset_ : 0.0;
        scale_ = datatype().is_integer() ? H.scale_ : 1.0;
        io.reset();
        return *this;
      }

      //! assignment operator from type of class derived from Header
      /*! This invokes the standard assignment operator=(const Header&). */
      template <class HeaderType, typename std::enable_if<std::is_base_of<Header, HeaderType>::value, void*>::type = nullptr>
        Header& operator= (const HeaderType& original) {
         return operator= (static_cast<const Header&> (original));
        }

      //! assignment operator from type of class other than Header
      /*! This copies all the relevant parameters over from \a original, */
      template <class HeaderType, typename std::enable_if<!std::is_base_of<Header, HeaderType>::value, void*>::type = nullptr>
        Header& operator= (const HeaderType& original) {
          axes_.resize (original.ndim());
          for (size_t n = 0; n < original.ndim(); ++n) {
            size(n) = original.size(n);
            stride(n) = original.stride(n);
            spacing(n) = original.spacing(n);
          }
          transform_ = original.transform();
          name_ = original.name();
          keyval_ = original.keyval();
          format_ = nullptr;
          datatype_ = DataType::from<typename HeaderType::value_type>();
          offset_ = 0.0;
          scale_ = 1.0;
          io.reset();
          return *this;
        }

      ~Header () { 
        if (io) {
          try { io->close (*this); }
          catch (Exception& E) {
            E.display();
          }
        }
      }

      bool valid () const { return bool (io); }
      bool operator! () const { return !valid(); }

      //! get the name of the image
      const std::string& name () const { return name_; }
      //! get/set the name of the image
      std::string& name () { return name_; }

      //! return the format of the image
      const char* format () const { return format_; }

      //! get the 4x4 affine transformation matrix mapping image to world coordinates
      const transform_type& transform () const { return transform_; }
      //! get/set the 4x4 affine transformation matrix mapping image to world coordinates
      transform_type& transform () { return transform_; }

      class NDimProxy { NOMEMALIGN
        public:
          NDimProxy (std::vector<Axis>& axes) : axes (axes) { }
          NDimProxy (NDimProxy&&) = default;
          NDimProxy (const NDimProxy&) = delete;
          NDimProxy& operator=(NDimProxy&&) = default;
          NDimProxy& operator=(const NDimProxy&) = delete;

          operator size_t () const { return axes.size(); }
          size_t   operator= (size_t new_size) { axes.resize (new_size); return new_size; }
          friend std::ostream& operator<< (std::ostream& stream, const NDimProxy& proxy) {
            stream << proxy.axes.size();
            return stream;
          }
        private:
          std::vector<Axis>& axes; 
      };

      //! return the number of dimensions (axes) of image
      size_t ndim () const { return axes_.size(); }
      //! set the number of dimensions (axes) of image
      NDimProxy ndim () { return { axes_ }; }

      //! get the number of voxels across axis
      const ssize_t& size (size_t axis) const;
      //! get/set the number of voxels across axis
      ssize_t& size (size_t axis);

      //! get the voxel size along axis
      const default_type& spacing (size_t axis) const;
      //! get/set the voxel size along axis
      default_type& spacing (size_t axis);

      //! get the stride between adjacent voxels along axis
      const ssize_t& stride (size_t axis) const;
      //! get/set the stride between adjacent voxels along axis
      ssize_t& stride (size_t axis);

      class DataTypeProxy : public DataType { NOMEMALIGN
        public:
          DataTypeProxy (Header& H) : DataType (H.datatype_), H (H) { }
          DataTypeProxy (DataTypeProxy&&) = default;
          DataTypeProxy (const DataTypeProxy&) = delete;
          DataTypeProxy& operator=(DataTypeProxy&&) = default;
          DataTypeProxy& operator=(const DataTypeProxy&) = delete;

          const uint8_t& operator()() const { return DataType::operator()(); }
          const DataType& operator= (const DataType& DT) { DataType::operator= (DT); set(); return *this; }
          void set_flag (uint8_t flag) { DataType::set_flag (flag); set(); }
          void unset_flag (uint8_t flag) { DataType::unset_flag (flag); set(); }
          void set_byte_order_native () { DataType::set_byte_order_native(); set(); }
        private:
          Header& H;
          void set () {
            H.datatype_ = dt;
            if (!is_integer())
              H.reset_intensity_scaling();
          }
      };

      //! get the datatype of the data as stored on file
      DataTypeProxy datatype () { return { *this }; }
      //! get/set the datatype of the data as stored on file
      DataType datatype () const { return datatype_; }

      //! get the offset applied to raw intensities
      default_type intensity_offset () const { return offset_; }
      //! get/set the offset applied to raw intensities
      default_type& intensity_offset () { return offset_; }
      //! get the scaling applied to raw intensities
      default_type intensity_scale () const { return scale_; }
      //! get/set the scaling applied to raw intensities
      default_type& intensity_scale () { return scale_; }

      //! update existing intensity offset & scale with values supplied
      void apply_intensity_scaling (default_type scaling, default_type bias = 0.0) {
        scale_ *= scaling;
        offset_ = scaling * offset_ + bias;
        set_intensity_scaling (scale_, offset_);
      }
      //! replace existing intensity offset & scale with values supplied
      void set_intensity_scaling (default_type scaling = 1.0, default_type bias = 0.0) {
        if (!std::isfinite (scaling) || !std::isfinite (bias) || scaling == 0.0)
          WARN ("invalid scaling parameters (offset: " + str(bias) + ", scale: " + str(scaling) + ")");
        scale_ = scaling;
        offset_ = bias;
      }
      //! replace existing intensity offset & scale with values in \a H
      void set_intensity_scaling (const Header& H) { set_intensity_scaling (H.intensity_scale(), H.intensity_offset()); }
      //! reset intensity offset to zero and scaling to one
      void reset_intensity_scaling () { set_intensity_scaling (); }

      bool is_file_backed () const { return valid() ? io->is_file_backed() : false; }

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
       * If \a read_write_if_existing is set, the data will be available
       * read-write even for input images (by default, these are opened
       * read-only). 
       *
       * \note this call will invalidate the invoking Header, by passing the
       * relevant internal structures into the Image produced. The Header will
       * no longer be valid(), and subsequent calls to get_image() will fail.
       * This done to ensure ownership of the internal data structures remains
       * clearly defined. 
       *
       * \warning do not modify the Header between its instantiation with the
       * open(), create() or scratch() calls, and obtaining an image via the
       * get_image() method. The latter will use the information in the Header
       * to access the data, and any mismatch in the information may cause
       * problems.
       */
      template <typename ValueType>
        Image<ValueType> get_image (bool read_write_if_existing = false);

      //! get generic key/value text attributes
      const std::map<std::string, std::string>& keyval () const { return keyval_; }
      //! get/set generic key/value text attributes
      std::map<std::string, std::string>& keyval () { return keyval_; }

      static Header open (const std::string& image_name);
      static Header create (const std::string& image_name, const Header& template_header);
      static Header scratch (const Header& template_header, const std::string& label = "scratch image");

      /*! use to prevent automatic realignment of transform matrix into
       * near-standard (RAS) coordinate system. */
      static bool do_not_realign_transform;

      //! return a string with the full description of the header
      std::string description() const;
      //! print out debugging information 
      friend std::ostream& operator<< (std::ostream& stream, const Header& H);

    protected:
      std::vector<Axis> axes_; 
      transform_type transform_; 
      std::string name_;
      std::map<std::string, std::string> keyval_;
      const char* format_;

      //! additional information relevant for images stored on file
      std::unique_ptr<ImageIO::Base> io; 
      //! the type of the data as stored on file
      DataType datatype_; 
      //! the values by which to scale the intensities
      default_type offset_, scale_;


      void acquire_io (Header& H) { io = std::move (H.io); }
      void merge (const Header& H);

      //! realign transform to match RAS coordinate system as closely as possible
      void realign_transform ();

      void sanitise_voxel_sizes ();
      void sanitise_transform ();
      void sanitise_strides () {
        Stride::sanitise (*this);
        Stride::actualise (*this);
      }
  };

  CHECK_MEM_ALIGN (Header);






  //! a class to hold attributes about each axis
  class Header::Axis { NOMEMALIGN
    public:
      Axis () noexcept : size (1), spacing (std::numeric_limits<default_type>::quiet_NaN()), stride (0) { }
      ssize_t size;
      default_type spacing;
      ssize_t stride;
  };








  inline const ssize_t& Header::size (size_t axis) const { return axes_[axis].size; }
  inline ssize_t& Header::size (size_t axis) { return axes_[axis].size; }

  inline const default_type& Header::spacing (size_t axis) const { return axes_[axis].spacing; }
  inline default_type& Header::spacing (size_t axis) { return axes_[axis].spacing; }

  inline const ssize_t& Header::stride (size_t axis) const { return axes_[axis].stride; }
  inline ssize_t& Header::stride (size_t axis) { return axes_[axis].stride; } 

  //! @}
}

#endif

