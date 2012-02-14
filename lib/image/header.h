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
#include "image/info.h"
#include "image/misc.h"
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

    namespace Handler { class Base; }

    class Header : public Info, public std::map<std::string, std::string>
    {
      public:
        Header () :
          format_ (NULL),
          offset_ (0.0),
          scale_ (1.0) { }

        //! constructor to open an image file.
        Header (const std::string& image_name) :
          Info (),
          format_ (NULL),
          offset_ (0.0),
          scale_ (1.0) {
          open (image_name);
        }

        Header (ConstInfo & H) :
          Info (H),
          format_ (NULL),
          offset_ (0.0),
          scale_ (1.0) { }

        Header& operator= (const Header& H) {
          Info::operator= (H);
          comments_ = H.comments();
          format_ = H.format();
          offset_ = H.intensity_offset();
          scale_ = H.intensity_scale();
          DW_scheme_ = H.DW_scheme_;
          return *this;
        }

        Header& operator= (const Info& H) {
          Info::operator= (H);
          return *this;
        }

        void set_info(const Info& H) {
          Info::operator= (H);
        }

        const std::vector<std::string>& comments () const {
          return comments_;
        }
        std::vector<std::string>& comments () {
          return comments_;
        }

        const char* format () const {
          return format_;
        }
        const char*& format () {
          return format_;
        }

        float intensity_offset () const {
          return offset_;
        }
        float& intensity_offset () {
          return offset_;
        }
        float intensity_scale () const {
          return scale_;
        }
        float& intensity_scale () {
          return scale_;
        }
        void apply_intensity_scaling (float scaling, float bias = 0.0) {
          scale_ *= scaling;
          offset_ = scaling * offset_ + bias;
        }


        const Math::Matrix<float>& DW_scheme () const {
          return DW_scheme_;
        }
        Math::Matrix<float>& DW_scheme () {
          return DW_scheme_;
        }

        std::string description () const;


        void clear () {
          Info::clear();
          std::map<std::string, std::string>::clear();
          comments_.clear();
          offset_ = 0.0;
          scale_ = 1.0;
          format_ = NULL;
          DW_scheme_.clear();
        }


        friend std::ostream& operator<< (std::ostream& stream, const Header& H)
        {
          stream << H.description();
          return stream;
        }

      protected:
        const char* format_;
        Math::Matrix<float> DW_scheme_;
        float offset_, scale_;
        std::vector<std::string> comments_;

        Handler::Base* open (const std::string& image_name, bool readwrite = false);
        Handler::Base* create (const std::string& image_name);
        void merge (const Header& H);

        template <class T> friend class Data;
    };






    class ConstHeader : public Header
    {
      public:
        ConstHeader () { }

        ConstHeader (const Header& H) : Header (H) { }

        //! constructor to open an image file.
        ConstHeader (const std::string& image_name) :
          Header (image_name) { }

        const std::string& name () const {
          return Header::name();
        }

        DataType datatype () const {
          return Header::datatype();
        }

        size_t ndim () const {
          return Header::ndim();
        }

        int dim (size_t axis) const {
          return Header::dim (axis);
        }

        float vox (size_t axis) const {
          return Header::vox (axis);
        }

        ssize_t stride (size_t axis) const {
          return Header::stride (axis);
        }

        const Math::Matrix<float>& transform () const {
          return Header::transform();
        }

        const std::vector<std::string>& comments () const {
          return Header::comments();
        }

        const char* format () const {
          return Header::format();
        }

        float intensity_offset () const {
          return Header::intensity_offset();
        }
        float intensity_scale () const {
          return Header::intensity_scale();
        }

        const Math::Matrix<float>& DW_scheme () const {
          return Header::DW_scheme();
        }

        std::string description () const {
          return Header::description();
        }

        friend std::ostream& operator<< (std::ostream& stream, const ConstHeader& H)
        {
          stream << static_cast<const Header&> (H);
          return stream;
        }

      private:
        template <class Set> void operator= (const Set& set) { assert (0); }
        using Header::set_ndim;
        using Header::clear;
        using Header::sanitise;
        using Header::apply_intensity_scaling;

    };

  }
  //! @}
}

#endif

