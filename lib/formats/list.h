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


#ifndef __formats_list_h__
#define __formats_list_h__

#include "header.h"

#define DECLARE_IMAGEFORMAT(format, desc) \
  class format : public Base { \
    public:  \
      format () : Base (desc) { } \
      virtual std::unique_ptr<ImageIO::Base> read (Header& H) const; \
      virtual bool check (Header& H, size_t num_axes) const; \
      virtual std::unique_ptr<ImageIO::Base> create (Header& H) const; \
  }

namespace MR
{

  //! Classes responsible for handling of specific image formats
  namespace Formats
  {

    //! the interface for classes that support the various image formats.
    /*! All image formats supported by %MRtrix are handled by a class derived
     * from the Formats::Base. An instance of each of these classes is
     * added to the list in the file list.cpp. */
    class Base
    {
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
         * false otherwise.  If true, this function should fill the Header \c
         * H with all the relevant information as read from the images before
         * returning.
         * \note this function should throw an Exception in case of error. */
        virtual std::unique_ptr<ImageIO::Base> read (Header& H) const = 0;

        /*! \brief check whether the Image::Header \c H can be created using
         * this handler.
         *
         * This function will check whether this handler can write images in
         * the format suggested by the filename. It will then check whether
         * the format can handle the number of dimensions requested, and
         * modify the header appropriately if needed.
         *
         * \return true if this format handler can write this type of file,
         * false otherwise.
         * \note this function should throw an Exception in case of error, or
         * if this image format cannot support the header information. */
        virtual bool check (Header& H, size_t num_axes) const = 0;

        /*! \brief create the image corresponding to the Image::Header \c H.
         *
         * This function will create images in the corresponding format,
         * assuming the header has been validated using the check() function
         * beforehand.
         *
         * \note this function should throw an Exception in case of error. */
        virtual std::unique_ptr<ImageIO::Base> create (Header& H) const = 0;
    };

#ifdef MRTRIX_AS_R_LIBRARY
    DECLARE_IMAGEFORMAT (RAM, "RAM buffer");
#endif

    DECLARE_IMAGEFORMAT (Pipe, "Internal pipe");
    DECLARE_IMAGEFORMAT (DICOM, "DICOM");
    DECLARE_IMAGEFORMAT (MRtrix, "MRtrix");
    DECLARE_IMAGEFORMAT (MRtrix_GZ, "MRtrix (GZip compressed)");
    DECLARE_IMAGEFORMAT (NIfTI1, "NIfTI-1.1");
    DECLARE_IMAGEFORMAT (NIfTI2, "NIfTI-2");
    DECLARE_IMAGEFORMAT (NIfTI1_GZ, "NIfTI-1.1 (GZip compressed)");
    DECLARE_IMAGEFORMAT (NIfTI2_GZ, "NIfTI-2 (GZip compressed)");
    DECLARE_IMAGEFORMAT (Analyse, "AnalyseAVW / NIfTI");
    DECLARE_IMAGEFORMAT (MRI, "MRTools (legacy format)");
    DECLARE_IMAGEFORMAT (XDS, "XDS");
    DECLARE_IMAGEFORMAT (MGH, "MGH");
    DECLARE_IMAGEFORMAT (MGZ, "MGZ (compressed MGH)");
    DECLARE_IMAGEFORMAT (MRtrix_sparse, "MRtrix WIP sparse image data format");

    /*! a list of all extensions for image formats that %MRtrix can handle. */
    extern const char* known_extensions[];

    /*! a list of all handlers for supported image formats. */
    extern const Base* handlers[];



    //! \cond skip
    extern MRtrix mrtrix_handler;
    //! \endcond

  }
}



#endif
