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

    //! \addtogroup Image 
    // @{
    
    class Axes {
      public:

        static const char*  left_to_right;
        static const char*  posterior_to_anterior;
        static const char*  inferior_to_superior;
        static const char*  time;
        static const char*  real_imag;
        static const char*  millimeters;
        static const char*  milliseconds;

        class Axis {
          public:
            Axis () : dim (1), vox (NAN), stride (0) { }

            int     dim;
            float   vox;
            ssize_t stride;
            std::string desc;
            std::string units;
        };

        class Dim {
          public:
            operator size_t () const { return (A.axes.size()); }
            size_t operator= (const Dim& C) { return (operator= (size_t(C))); }
            size_t operator= (size_t value) { A.set_ndim (value); return (value); }
            size_t operator+= (size_t value) { value += A.axes.size(); A.set_ndim (value); return (value); }
            size_t operator-= (size_t value) { value = A.axes.size() - value; A.set_ndim (value); return (value); }
            size_t operator*= (size_t value) { value *= A.axes.size(); A.set_ndim (value); return (value); }
            size_t operator/= (size_t value) { value = A.axes.size() / value; A.set_ndim (value); return (value); }
          private:
            Dim (Axes& parent) : A (parent) { }
            Axes& A;
            friend class Axes;
        };

        Axes () { }
        Axes (size_t ndim) : axes (ndim) { set_default_axes (0); }

        const std::string& name () const { return (axes_name); }

        void clear () { axes.clear(); }
        void sanitise ();
        void get_strides (size_t& start, std::vector<ssize_t>& stride) const;

        Axis&       operator[] (size_t index)       { return (axes[index]); }
        const Axis& operator[] (size_t index) const { return (axes[index]); }

        // DataSet interface:
        size_t       ndim () const { return (axes.size()); }
        Dim          ndim ()       { return (Dim (*this)); }

        int  dim (size_t index) const { return (axes[index].dim); }
        int& dim (size_t index)       { return (axes[index].dim); }

        float  vox (size_t index) const { return (axes[index].vox); }
        float& vox (size_t index)       { return (axes[index].vox); }

        const std::string& description (size_t index) const { return (axes[index].desc); }
        std::string&       description (size_t index)       { return (axes[index].desc); }

        const std::string& units (size_t index) const { return (axes[index].units); }
        std::string&       units (size_t index)       { return (axes[index].units); }

        ssize_t  stride (size_t index) const { return (axes[index].stride); }
        ssize_t& stride (size_t index)       { return (axes[index].stride); }

        bool  forward (size_t index) const { return (axes[index].stride > 0); }
        ssize_t direction (size_t index) const { return (axes[index].stride > 0 ? 1 : -1); }

        friend std::ostream& operator<< (std::ostream& stream, const Axes& axes);
        static std::vector<ssize_t> parse (size_t ndim, const std::string& specifier);
        static void check (const std::vector<ssize_t>& parsed, size_t ndim);

      protected:
        std::vector<Axis> axes;
        static const std::string axes_name;

        void set_ndim (size_t number_of_dims) { size_t from = ndim(); axes.resize (number_of_dims); set_default_axes (from); }

        void set_default_axes (size_t from) { 
          for (size_t n = from; n < ndim(); n++) {
            Axis& a (axes[n]);
            switch (n) {
              case 0: a.desc = left_to_right; a.units = millimeters; break;
              case 1: a.desc = posterior_to_anterior; a.units = millimeters; break;
              case 2: a.desc = inferior_to_superior; a.units = millimeters; break;
              default: a.desc.clear(); a.units.clear();
            }
          }
        }

        ssize_t find_free_axis () const {
          for (size_t a = 1; a <= ndim(); a++) {
            size_t m = 0;
            for (; m < ndim(); m++) if (abs(axes[m].stride) == a) break; 
            if (m >= ndim()) return (a);
          }
          return (0);
        }
    };
    
    //! @}

  }
}

#endif


