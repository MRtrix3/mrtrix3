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

#ifndef __image_axis_h__
#define __image_axis_h__

#include "types.h"
#include "mrtrix.h"

namespace MR {
  namespace Image {

    class Axis {
      public:
        Axis () : dim (1), vox (NAN), stride (0) { }

        int     dim;
        float   vox;
        ssize_t stride;
        std::string description;
        std::string units;

        bool  forward () const { return (stride > 0); }
        ssize_t direction () const { return (stride > 0 ? 1 : -1); }

        friend std::ostream& operator<< (std::ostream& stream, const Axis& axes);

        static std::vector<ssize_t> parse (size_t ndim, const std::string& specifier);
        static void check (const std::vector<ssize_t>& parsed, size_t ndim);

        static const char*  left_to_right;
        static const char*  posterior_to_anterior;
        static const char*  inferior_to_superior;
        static const char*  time;
        static const char*  real_imag;
        static const char*  millimeters;
        static const char*  milliseconds;
    };

    //! @}

  }
}

#endif


