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


    17-03-2009 J-Donald Tournier <d.tournier@brain.org.au>
    * additional sanity checks in sanitise_transform(): 
    * - make sure voxel sizes are finite numbers
    * - make sure all entries in the transform matrix are finite.
    * use sane defaults otherwise.

*/

#include "app.h"
#include "image/header.h"
#include "image/misc.h"
#include "math/matrix.h"
#include "math/permutation.h"

namespace MR {
  namespace Image {

    void Header::sanitise ()
    {
      debug ("sanitising header...");

      if (axes.ndim() < 3) {
        info ("image contains fewer than 3 dimensions - adding extra dimensions");
        axes.resize (3);
      }

      if (!finite (axes.vox(0)) || !finite (axes.vox(1)) || !finite (axes.vox(2))) {
        error ("invalid voxel sizes - resetting to sane defaults");
        axes.vox(0) = axes.vox(1) = axes.vox(2) = 1.0;
      }

      if (transform_matrix.is_set()) {
        if (transform_matrix.rows() != 4 || transform_matrix.columns() != 4) {
          transform_matrix.clear();
          error ("transform matrix is not 4x4 - resetting to sane defaults");
        }
        else {
          for (size_t i = 0; i < 3; i++) {
            for (size_t j = 0; j < 4; j++) {
              if (!finite (transform_matrix(i,j))) {
                transform_matrix.clear();
                error ("transform matrix contains invalid entries - resetting to sane defaults");
                break;
              }
            }
            if (!transform_matrix.is_set()) break;
          }
        }
      }

      if (!transform_matrix.is_set()) {
        transform_matrix.allocate (4,4);
        transform_matrix.identity();
        transform_matrix(0,3) = -0.5 * axes.dim(0) * axes.vox(0);
        transform_matrix(1,3) = -0.5 * axes.dim(1) * axes.vox(1);
        transform_matrix(2,3) = -0.5 * axes.dim(2) * axes.vox(2);
      }

      transform_matrix(3,0) = transform_matrix(3,1) = transform_matrix(3,2) = 0.0; transform_matrix(3,3) = 1.0;

      Math::Permutation permutation (3);
      Math::absmax (transform_matrix.row(0).view(0,3), permutation[0]);
      Math::absmax (transform_matrix.row(1).view(0,3), permutation[1]);
      Math::absmax (transform_matrix.row(2).view(0,3), permutation[2]);

      bool flip[3] = {
        ( transform_matrix(0, permutation[0]) < 0.0 ),
        ( transform_matrix(1, permutation[1]) < 0.0 ),
        ( transform_matrix(2, permutation[2]) < 0.0 )
      };

      if (permutation[0] != 0 || permutation[1] != 1 || permutation[2] != 2 || flip[0] || flip[1] || flip[2]) {

        std::vector<Axes::Axis> a;
        a.push_back (axes[permutation[0]]);
        a.push_back (axes[permutation[1]]);
        a.push_back (axes[permutation[2]]);

        for (size_t i = 0; i < 3; i++) {
          Math::VectorView<float> row = transform_matrix.row(i).view(0,3);
          permutation.apply (row); 
        }

        for (size_t i = 0; i < 3; i++) {
          Math::VectorView<float> translation = transform_matrix.column(4).view(0,3);
          if (flip[i]) {
            a[i].forward = !a[i].forward;
            float length = (a[i].dim-1) * a[i].vox;
            Math::VectorView<float> axis = transform_matrix.column(i).view(0,3);
            for (size_t n = 0; n < 3; n++) {
              axis[n] = -axis[n];
              translation[n] += length*axis[n];
            }
          }
        }
      }

      num_voxel_per_file = voxel_count (*this) * ( dtype.is_complex() ? 2 : 1 ) / files.size();

      debug ("setting up data increments for \"" + name + "\"...");

      first_voxel_offset = 0;

      size_t order[ndim()];
      size_t last = ndim()-1;
      for (size_t i = 0; i < ndim(); i++) {
        if (axes.order(i) != Axes::undefined) order[axes.order(i)] = i; 
        else order[last--] = i;
      }

      size_t axis;
      ssize_t mult = 1;

      for (size_t i = 0; i < ndim(); i++) {
        axis = order[i];
        assert (axis < ndim());
        if (axes.stride(axis)) throw Exception ("invalid data order specifier for image \"" + name + "\": same dimension specified twice");

        axes.stride(axis) = mult * ssize_t(axes.direction (axis));
        if (axes.stride(axis) < 0) first_voxel_offset += size_t(-axes.stride(axis)) * size_t(dim(axis)-1);
        mult *= ssize_t(dim(axis));
      }

      if (dtype.is_complex()) {
        first_voxel_offset *= 2;
        for (size_t i = 0; i < ndim(); i++) axes.stride(i) *= 2;
      }


      if (App::log_level > 2) {
        std::string string ("data increments initialised with start = " + str (first_voxel_offset) + ", stride = [ ");
        for (size_t i = 0; i < ndim(); i++) string += str (axes.stride(i)) + " "; 
        debug (string + "]");
      }
    }










    std::string Header::description() const
    {
      std::string desc ( 
            "************************************************\n"
            "Image:               \"" + name + "\"\n"
            "************************************************\n"
            "  Format:            " + ( format ? format : "undefined" ) + "\n"
            "  Dimensions:        ");

      size_t i;
      for (i = 0; i < axes.ndim(); i++) {
        if (i) desc += " x ";
        desc += str (axes.dim(i));
      }



      desc += "\n  Voxel size:        ";

      for (i = 0; i < axes.ndim(); i++) {
        if (i) desc += " x ";
        desc += isnan (axes.vox(i)) ? "?" : str (axes.vox(i));
      }




      desc += "\n  Dimension labels:  ";

      for (i = 0; i < axes.ndim(); i++)  
        desc += ( i ? "                     " : "" ) + str (i) + ". " 
          + ( axes.description(i).size() ? axes.description(i) : "undefined" ) + " ("
          + ( axes.units(i).size() ? axes.units(i) : "?" ) + ")\n";



      desc += std::string ("  Data type:         ") + ( dtype.description() ? dtype.description() : "invalid" ) + "\n"
            "  Data layout:       [ ";


      for (i = 0; i < axes.ndim(); i++) 
        desc += axes.order(i) == Axes::undefined ? "? " : ( axes.forward(i) ? '+' : '-' ) + str (axes.order(i)) + " ";



      desc += "]\n"
            "  Data scaling:      offset = " + str (offset) + ", multiplier = " + str (scale) + "\n"
            "  Comments:          " + ( comments.size() ? comments[0] : "(none)" ) + "\n";



      for (i = 1; i < comments.size(); i++)
        desc += "                     " + comments[i] + "\n";

      if (size() > 0) {
        desc += "  Properties:\n";
        for (std::map<std::string, std::string>::const_iterator it = begin(); it != end(); ++it)
          desc += "    " + it->first + ": " + it->second + "\n";
      }

      if (transform_matrix.is_set()) {
        desc += "  Transform:         ";
        for (size_t i = 0; i < transform_matrix.rows(); i++) {
          if (i) desc +=  "                     ";
          for (size_t j = 0; j < transform_matrix.columns(); j++) {
            char buf[14], buf2[14];
            snprintf (buf, 14, "%.4g", transform_matrix(i,j));
            snprintf (buf2, 14, "%12.10s", buf);
            desc += buf2;
          }
          desc += "\n";

        }
      }

      if (DW_scheme.is_set()) 
        desc += "  DW scheme:         " + str (DW_scheme.rows()) + " x " + str (DW_scheme.columns()) + "\n";

      return (desc);
    }



    std::ostream& operator<< (std::ostream& stream, const Header& H)
    {
      stream << H.description();
      return (stream);
    }

  }
}
