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

#ifndef __file_ximg_h__
#define __file_ximg_h__

#include "get_set.h"
#include "file/mmap.h"


namespace MR {
  namespace File {

    class XImg {
      public:
        void           read (const std::string& filename);
        std::string    name () const;

        const uint8_t* pixel_data () const;
        int            width () const;
        int            height () const;
        int            depth () const;

        friend std::ostream& operator<< (std::ostream& stream, const XImg& X);

      private:
        MMap           mmap;
        const uint8_t* bof () const;
    };












    inline const uint8_t*  XImg::bof () const { return ((uint8_t*) mmap.address()); }

    inline void XImg::read (const std::string& filename) 
    {
      mmap.init (filename);
      mmap.map();
    }

    inline std::string XImg::name () const { return (mmap.name()); }

    inline const uint8_t* XImg::pixel_data () const { return (bof() + getBE<int32_t> (bof() + 0x4)); }
    inline int XImg::width () const      { return (getBE<int32_t> (bof() + 0x8)); }
    inline int XImg::height () const     { return (getBE<int32_t> (bof() + 0xc)); }
    inline int XImg::depth () const      { return (getBE<int32_t> (bof() + 0x10)); }

    inline std::ostream& operator<< (std::ostream& stream, const XImg& X) 
    {
      stream << "name: \"" << X.name() << ", pixel_data at " << size_t (X.pixel_data() - X.bof()) << ", dim: [ " << X.width() << " " << X.height() << " ]\n";
      return (stream);
    }

  }
}

#endif



