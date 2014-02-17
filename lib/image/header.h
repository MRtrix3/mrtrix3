/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


#ifndef __image_header_h__
#define __image_header_h__

#include <map>

#include "debug.h"
#include "ptr.h"
#include "types.h"
#include "datatype.h"
#include "image/info.h"
#include "image/handler/base.h"
#include "image/utils.h"
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

        //! copy constructor
        /*! This copies everything over apart from the handler and the
         * intensity scaling. */
        Header (const Header& H) :
          Info (H.info()),
          std::map<std::string, std::string> (H),
          format_ (H.format_),
          DW_scheme_ (H.DW_scheme_),
          offset_ (0.0),
          scale_ (1.0),
          comments_ (H.comments_) { }

        Header& operator= (const Header& H) {
          Info::operator= (H);
          std::map<std::string, std::string>::operator= (H);
          comments_ = H.comments_;
          format_ = H.format_;
          offset_ = 0.0;
          scale_ = 1.0;
          DW_scheme_ = H.DW_scheme_;
          handler_ = NULL;
          return *this;
        }

        const Info& info() const {
          return (*this);
        }

        Info& info() {
          return (*this);
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
        void set_intensity_scaling (float scaling = 1.0, float bias = 0.0) {
          scale_ = scaling;
          offset_ = bias;
        }
        void set_intensity_scaling (const Header& H) {
          set_intensity_scaling (H.intensity_scale(), H.intensity_offset());
        }
        void reset_intensity_scaling () {
          set_intensity_scaling ();
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

        RefPtr<Handler::Base> __get_handler () const {
          return handler_;
        }

        void __set_handler (RefPtr<Handler::Base> handler) {
          handler_ = handler;
        }

      protected:
        const char* format_;
        Math::Matrix<float> DW_scheme_;
        float offset_, scale_;
        std::vector<std::string> comments_;
        RefPtr<Handler::Base> handler_;

        void open (const std::string& image_name);
        void create (const std::string& image_name);
        void merge (const Header& H);

        template <class ValueType> friend class Buffer;

    };






    class ConstHeader : public Header
    {
      public:
        ConstHeader () { }

        ConstHeader (const Header& H) : Header (H) { }

        //! constructor to open an image file.
        ConstHeader (const std::string& image_name) :
          Header (image_name) { }

        const Info& info () const {
          return *this;
        }

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

