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

#ifndef __image_format_list_h__
#define __image_format_list_h__

#define DECLARE_IMAGEFORMAT(format) \
  class format : public Base { \
    public:  \
      format () : Base (#format) { } \
    protected: \
      virtual bool   read (Header& H) const; \
      virtual bool   check (Header& H, int num_axes = 0) const; \
      virtual void   create (Header& H) const; \
  }


namespace MR {
  namespace Image {

    class Header;

    //! \addtogroup Image 
    // @{

    namespace Format {

      /*! the interface for classes that support the various image formats.
       * 
       * All image formats supported by %MRtrix are handled by a class derived
       * from the Image::Format::Base. An instance of each of these classes is
       * added to the list in the file list.cpp. */
      class Base {
        public:
          Base (const char* desc) : description (desc) { }
          virtual ~Base() { }

          const char*  description; //!< a short human-readable description of the image format

          /*! \brief read image file(s) and fill the Image::Header \c H with the
           * appropriate information.
           *
           * This function will check whether this handler can read images in
           * the format suggested by the filename. It will then attempt to read
           * the corresponding image, and update the Image::Header \c H with
           * the relevant information. 
           * 
           * \return true if this Format handler can read this type of file,
           * false otherwise. 
           * \note this function should throw an Exception in case of error. */
          virtual bool read (Header& H) const = 0;

          /*! \brief check whether the Image::Header \c H can be created using
           * this handler. 
           *
           * This function will check whether this handler can write images in
           * the format suggested by the filename. It will then check whether
           * the format can handle the number of dimensions requested, and
           * modify the header appropriately if needed.
           *
           * \return true if this Format handler can write this type of file,
           * false otherwise. 
           * \note this function should throw an Exception in case of error, or
           * if this image format cannot support the header information. */
          virtual bool check (Header& H, int num_axes = 0) const = 0;

          /*! \brief create the image corresponding to the Image::Header \c H.
           *
           * This function will check whether this handler can create images in
           * the format suggested by the filename. It will then check whether
           * the format can handle the number of dimensions requested, and
           * modify the header appropriately if needed.
           *
           * \note this function should throw an Exception in case of error. */
          virtual void create (Header& H) const = 0;
      };

      DECLARE_IMAGEFORMAT (Analyse);
      DECLARE_IMAGEFORMAT (NIfTI);
      DECLARE_IMAGEFORMAT (MRI);
      DECLARE_IMAGEFORMAT (DICOM);
      DECLARE_IMAGEFORMAT (XDS);
      DECLARE_IMAGEFORMAT (MRtrix);


      /*! a list of all extensions for image formats that %MRtrix can handle. */
      extern const char* known_extensions[];

      /*! a list of all handlers for supported image formats. */
      extern const Base* handlers[]; 

    }
    //! @}
  }
}



#endif
