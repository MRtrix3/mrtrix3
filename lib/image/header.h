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

#ifndef __image_header_h__
#define __image_header_h__

#include <map>

#include "debug.h"
#include "ptr.h"
#include "types.h"
#include "data_type.h"
#include "image/axis.h"
#include "dataset/misc.h"
#include "image/handler/base.h"
#include "file/mmap.h"
#include "math/matrix.h"

namespace MR
{
  /*! \defgroup ImageAPI Image access
   * \brief Classes and functions providing access to image data. */
  // @{

  //! functions and classes related to image data input/output
  namespace Image
  {

    //! A container for all the information related to an image
    /*! This class contains all the information available about an image as it
     * is (or will be) stored on disk. It does not provide access to the voxel
     * data themselves. For this, please use the Image::Voxel class.
     *
     * The Header class provides a limited implementation of the DataSet
     * interface. As such, it can already be used with a number of the relevant
     * algorithms. */
    class Header : public std::map<std::string, std::string>
    {
      public:
        //! default constructor
        Header () : format_ (NULL), offset_ (0.0), scale_ (1.0), readwrite_ (false) { }

        //! constructor to open an image file.
        /*! This constructor will open an existing image, using the Header::open()
         * function. */
        Header (const std::string& image_name) :
          format_ (NULL), offset_ (0.0), scale_ (1.0), readwrite_ (false) {
          open (image_name);
        }

        //! copy constructor
        Header (const Header& H) :
          std::map<std::string, std::string> (H), format_ (NULL), dtype_ (H.dtype_),
          transform_ (H.transform_), DW_scheme_ (H.DW_scheme_), axes_ (H.axes_),
          offset_ (H.offset_), scale_ (H.scale_), readwrite_ (false), comments_ (H.comments_) { }

        //! assignment operator
        Header& operator= (const Header& H) {
          check_not_finalised();
          std::map<std::string, std::string>::operator= (H);
          format_ = NULL;
          offset_ = H.offset_;
          scale_ = H.scale_;
          readwrite_ = false;
          comments_ = H.comments();
          set_transform (H.transform());
          set_DW_scheme (H.DW_scheme());
          axes_ = H.axes_;
          return (*this);
        }

        //! assignment operator from DataSet
        /*! This will fill the dimensions, voxel size, and transform parts of
         * the Header according to those of the DataSet \a ds, leaving all
         * other parts of the Header unmodified. */
        template <class DataSet> Header& operator= (const DataSet& ds) {
          check_not_finalised();
          set_transform (ds.transform());
          set_ndim (ds.ndim());
          for (size_t i = 0; i < ds.ndim(); i++) {
            set_dim (i, ds.dim (i));
            set_vox (i, ds.vox (i));
          }
          return (*this);
        }

        //! check whether Header is ready to access data
        bool operator! () const {
          return (handler_);
        }

        const std::string& name () const {
          return (name_);
        }
        const std::vector<std::string>& comments () const {
          return (comments_);
        }
        const char* format () const {
          return (format_);
        }
        bool readwrite () const {
          return (readwrite_);
        }
        const DataType& datatype () const {
          return (dtype_);
        }
        bool is_complex () const {
          return (dtype_.is_complex());
        }

        float data_offset () const {
          return (offset_);
        }
        float data_scale () const {
          return (scale_);
        }

        size_t ndim () const {
          return (axes_.size());
        }
        int dim (size_t index) const {
          return (axes_[index].dim);
        }
        float vox (size_t index) const {
          return (axes_[index].vox);
        }
        ssize_t stride (size_t axis) const {
          return (axes_[axis].stride);
        }
        const std::string& description (size_t axis) const {
          return (axes_[axis].description);
        }
        const std::string& units (size_t axis) const {
          return (axes_[axis].units);
        }

        const Math::Matrix<float>& DW_scheme () const {
          return (DW_scheme_);
        }
        const Math::Matrix<float>& transform () const {
          return (transform_);
        }

        std::string  description () const;


        // COMMITTERS:

        //! open the image specified
        /*! Open an existing image stored as \a image_name, and fill the Header
         * with the corresponding information.
         * \note after this call, the header should not be modified any
         * further. In debug mode, this will cause the program to abort. */
        void open (const std::string& image_name);

        //! create a new image
        /*! Creates a new image with the name \a image_name, according to the
         * settings currently stored in the Header.
         * \note after this call, the header should not be modified any
         * further. In debug mode, this will cause the program to abort. */
        void create (const std::string& image_name);



        // MODIFIERS:
        // These functions should not be called after open() or create().

        void set_name (const std::string& new_name) {
          name_ = new_name;
        }
        void set_datatype (DataType new_datatype) {
          check_not_finalised();
          dtype_ = new_datatype;
        }
        void set_datatype (const std::string& specifier) {
          check_not_finalised();
          dtype_.parse (specifier);
        }
        void set_transform (const Math::Matrix<float>& new_transform) {
          check_not_finalised();
          transform_ = new_transform;
        }
        void set_DW_scheme (const Math::Matrix<float>& new_DW_scheme) {
          check_not_finalised();
          DW_scheme_ = new_DW_scheme;
        }

        //! override header parameters defined by a DataSet
        /*! This will override the dimensions, voxel size, stride, and transform
         * parts of the Header according to those of the DataSet \a ds, leaving all
         * other parts of the Header unmodified. */
        template <class DataSet> void set_params (const DataSet& ds) {
          check_not_finalised();
          set_transform (ds.transform());
          set_ndim (ds.ndim());
          for (size_t i = 0; i < ds.ndim(); i++) {
            set_dim (i, ds.dim (i));
            set_vox (i, ds.vox (i));
          }
        }

        void add_comment (const std::string& text) {
          check_not_finalised();
          comments_.push_back (text);
        }

        Math::Matrix<float>& get_DW_scheme () {
          check_not_finalised();
          return (DW_scheme_);
        }
        Math::Matrix<float>& get_transform () {
          check_not_finalised();
          return (transform_);
        }
        std::vector<std::string>& get_comments () {
          check_not_finalised();
          return (comments_);
        }

        void set_ndim (size_t new_ndim) {
          check_not_finalised();
          axes_.resize (new_ndim);
        }
        void set_dim (size_t axis, int new_dim) {
          check_not_finalised();
          axes_[axis].dim = new_dim;
        }
        void set_vox (size_t axis, float new_vox) {
          check_not_finalised();
          axes_[axis].vox = new_vox;
        }
        void set_stride (size_t axis, ssize_t new_stride) {
          check_not_finalised();
          axes_[axis].stride = new_stride;
        }
        void set_description (size_t axis, const std::string& new_description) {
          check_not_finalised();
          axes_[axis].description = new_description;
        }
        void set_units (size_t axis, const std::string& new_units) {
          check_not_finalised();
          axes_[axis].units = new_units;
        }

        void clear () {
          std::map<std::string, std::string>::clear();
          name_.clear();
          axes_.clear();
          comments_.clear();
          dtype_ = DataType();
          offset_ = 0.0;
          scale_ = 1.0;
          readwrite_ = false;
          format_ = NULL;
          transform_.clear();
          DW_scheme_.clear();
        }

        void  set_scaling (float scaling = 1.0, float bias = 0.0) {
          check_not_finalised();
          offset_ = bias;
          scale_ = scaling;
        }
        void  reset_scaling () {
          set_scaling();
        }
        void  apply_scaling (float scaling, float bias = 0.0) {
          check_not_finalised();
          scale_ *= scaling;
          offset_ = scaling * offset_ + bias;
        }


        // USED IN BACKEND:

        void  set_handler (Handler::Base* handler) {
          handler_ = handler;
        }
        Handler::Base* get_handler () {
          return (handler_);
        }
        void add_file (const File::Entry& entry) {
          files_.push_back (entry);
        }
        const std::vector<File::Entry>& get_files () const {
          return (files_);
        }


        // HELPERS:

        template <typename T> static inline
        T scale_from_storage (T val, float scale_f, float offset_f) {
          return (offset_f + scale_f * val);
        }

        template <typename T> static inline
        T scale_to_storage (T val, float scale_f, float offset_f) {
          return ( (val - offset_f) / scale_f);
        }

        template <typename T>
        float scale_from_storage (T val) const {
          return (scale_from_storage (val, scale_, offset_));
        }

        template <typename T>
        float scale_to_storage (T val) const   {
          return (scale_to_storage (val, scale_, offset_));
        }

        //! returns the memory footprint of the Image
        int64_t footprint (size_t from_dim = 0, size_t up_to_dim = std::numeric_limits<size_t>::max()) {
          return (footprint_for_count (DataSet::voxel_count (*this, from_dim, up_to_dim)));
        }

        //! returns the memory footprint of a DataSet
        int64_t footprint (const char* specifier) {
          return (footprint_for_count (DataSet::voxel_count (*this, specifier)));
        }

        friend std::ostream& operator<< (std::ostream& stream, const Header& H);

      private:
        const char*          format_;
        std::string          name_;
        DataType             dtype_;
        Math::Matrix<float>  transform_, DW_scheme_;
        std::vector<Axis>    axes_;
        float                offset_, scale_;
        bool                 readwrite_;
        std::vector<std::string>   comments_;
        std::vector<File::Entry>   files_;
        Ptr<Handler::Base>   handler_;

        void sanitise ();
        void merge (const Header& H);
        void check_not_finalised () const {
          assert (files_.empty() && !handler_);
        }

        int64_t footprint_for_count (int64_t count) {
          return (dtype_ == DataType::Bit ? (count+7) /8 : count * dtype_.bytes());
        }

    };

  }
  //! @}
}

#endif

