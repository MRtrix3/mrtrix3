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

#include "app.h"
#include "image/axis.h"

namespace MR {
  namespace Image {

    const std::string Axes::axes_name ("anonymous axes");
    const char* Axes::left_to_right = "left->right";
    const char* Axes::posterior_to_anterior = "posterior->anterior";
    const char* Axes::inferior_to_superior = "inferior->superior";
    const char* Axes::time = "time";
    const char* Axes::real_imag = "real-imaginary";
    const char* Axes::millimeters = "mm";
    const char* Axes::milliseconds = "ms";



    void Axes::sanitise ()
    {
      // remove unset/invalid axis orderings:
      for (size_t a = 0; a < ndim(); a++) 
        if (order(a) >= ndim()) 
          order(a) = find_free_axis();

      // remove duplicates:
      for (size_t a = 1; a < ndim(); a++) {
        for (size_t n = 0; n < a; n++) {
          if (order(a) == order(n)) { 
            order(a) = find_free_axis();
            break; 
          }
        }
      }
    }





    void Axes::get_strides (size_t& start, std::vector<ssize_t>& stride) const
    {
      start = 0;
      stride.resize (ndim(), 0);

      ssize_t mult = 1;
      for (size_t i = 0; i < ndim(); i++) {
        size_t axis = order(i);
        assert (axis < ndim());
        assert (!stride[axis]);
        stride[axis] = mult * ssize_t(direction(axis));
        if (stride[axis] < 0) start += size_t(-stride[axis]) * size_t(dim(axis)-1);
        mult *= ssize_t(dim(axis));
      }

      if (App::log_level > 2) {
        std::string string ("data strides initialised with start = " + str (start) + ", stride = [ ");
        for (size_t i = 0; i < ndim(); i++) string += str (stride[i]) + " "; 
        debug (string + "]");
      }
    }






    std::vector<Axes::Order> parse_axes_specifier (const Axes& original, const std::string& specifier)
    {
      std::vector<Axes::Order> parsed (original.ndim());

      size_t str = 0;
      size_t lim = 0;
      size_t end = specifier.size();
      size_t current = 0;

      try {
        while (str <= end) {
          parsed[current].forward = original.forward(current);
          if (specifier[str] == '+') { parsed[current].forward = true; str++; }
          else if (specifier[str] == '-') { parsed[current].forward = false; str++; }
          else if (!( specifier[str] == '\0' || specifier[str] == ',' ) && !isdigit (specifier[str])) throw 0;

          lim = str;

          if (specifier[str] == '\0' || specifier[str] == ',') parsed[current].order = original.order (current);
          else {
            while (isdigit (specifier[lim])) lim++;
            if (specifier[lim] != ',' && specifier[lim] != '\0') throw 0;
            parsed[current].order = to<size_t> (specifier.substr (str, lim-str));
          }

          str = lim+1;
          current++;
        }
      }
      catch (int) { throw Exception ("malformed axes specification \"" + specifier + "\""); }
      catch (...) { throw; }

      if (current != original.ndim()) 
        throw Exception ("incorrect number of axes in axes specification \"" + specifier + "\"");

      check_axes_specifier (parsed, original.ndim());

      return (parsed);
    }





    void check_axes_specifier (const std::vector<Axes::Order>& parsed, size_t ndim)
    {
      for (size_t n = 0; n < parsed.size(); n++) {
        if (parsed[n].order >= ndim) 
          throw Exception ("axis ordering " + str (parsed[n].order) + " out of range");

        for (size_t i = 0; i < n; i++) 
          if (parsed[i].order == parsed[n].order) 
            throw Exception ("duplicate axis ordering (" + str (parsed[n].order) + ")");
      }
    }







    std::ostream& operator<< (std::ostream& stream, const Axes& axes)
    {
      stream << "dim [ ";
      for (size_t n = 0; n < axes.ndim(); n++) stream << axes.dim(n) << " ";
      stream << "], vox [ ";
      for (size_t n = 0; n < axes.ndim(); n++) stream << axes.vox(n) << " ";
      stream << "], order [ ";
      for (size_t n = 0; n < axes.ndim(); n++) stream << ( axes.forward(n) ? '+' : '-' ) << axes.order(n) << " ";
      stream << "], desc [ ";
      for (size_t n = 0; n < axes.ndim(); n++) stream << "\n" << axes.description(n) << "\" ";
      stream << "], units [ ";
      for (size_t n = 0; n < axes.ndim(); n++) stream << "\n" << axes.units(n) << "\" ";

      return (stream);
    }




  }
}

