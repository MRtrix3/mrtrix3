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

#include "image/format/base.h"

#define DECLARE_IMAGEFORMAT(format) \
  class format : public Base { \
    public:  \
      format () : Base (#format) { } \
    protected: \
      virtual bool   read (Header& H) const; \
      virtual bool   check (Header& H, int num_axes = 0) const; \
      virtual void   create (const Header& H) const; \
  }


namespace MR {
  namespace Image {
    namespace Format {

      extern const Base* handlers[]; 

      DECLARE_IMAGEFORMAT (Analyse);
      DECLARE_IMAGEFORMAT (NIfTI);
      DECLARE_IMAGEFORMAT (MRI);
      DECLARE_IMAGEFORMAT (DICOM);
      DECLARE_IMAGEFORMAT (XDS);
      DECLARE_IMAGEFORMAT (MRtrix);

    }
  }
}



#endif
