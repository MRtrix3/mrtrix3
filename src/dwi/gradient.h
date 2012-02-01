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

#ifndef __dwi_gradient_h__
#define __dwi_gradient_h__

#include "app.h"
#include "point.h"
#include "file/path.h"
#include "image/header.h"
#include "math/LU.h"
#include "math/matrix.h"

namespace MR
{
  namespace App { class OptionGroup; }

  namespace DWI
  {
    extern const App::OptionGroup GradOption;


    template <typename T> 
      Math::Matrix<T>& normalise_grad (Math::Matrix<T>& grad)
    {
      if (grad.columns() != 4)
        throw Exception ("invalid gradient matrix dimensions");
      for (size_t i = 0; i < grad.rows(); i++) {
        T norm = grad (i,3) ?
                 T (1.0) /Math::norm (grad.row (i).sub (0,3)) :
                 T (0.0);
        grad.row (i).sub (0,3) *= norm;
      }
      return (grad);
    }


    template <typename T> 
      inline void guess_DW_directions (
          std::vector<int>& dwi, 
          std::vector<int>& bzero, 
          const Math::Matrix<T>& grad)
    {
      if (grad.columns() != 4)
        throw Exception ("invalid gradient encoding matrix: expecting 4 columns.");
      dwi.clear();
      bzero.clear();
      for (size_t i = 0; i < grad.rows(); i++) {
        if (grad (i,3))
          dwi.push_back (i);
        else
          bzero.push_back (i);
      }

      info ("found " + str (dwi.size()) + " diffusion-weighted volumes and " 
          + str (bzero.size()) + " b=0 volumes");
    }




    template <typename T> 
      inline Math::Matrix<T>& gen_direction_matrix (
          Math::Matrix<T>& dirs, 
          const Math::Matrix<T>& grad, 
          const std::vector<int>& dwi)
    {
      dirs.allocate (dwi.size(),2);
      for (size_t i = 0; i < dwi.size(); i++) {
        T n = Math::norm (grad.row (dwi[i]).sub (0,3));
        dirs (i,0) = Math::atan2 (grad (dwi[i],1), grad (dwi[i],0));
        dirs (i,1) = Math::acos (grad (dwi[i],2) /n);
      }
      return dirs;
    }


    template <typename T> Math::Matrix<T> get_DW_scheme (const Image::Header& dwi_header)
    {
      using namespace App;
      if (dwi_header.ndim() != 4)
        throw Exception ("dwi image should contain 4 dimensions");

      Math::Matrix<T> grad;

      Options opt = get_options ("grad");
      if (opt.size()) {
        const std::string grad_file_name = Path::basename (opt[0][0]);
        if (grad_file_name == "bvals" || grad_file_name == "bvecs")
          load_bvecs_bvals (Path::dirname (opt[0][0]), grad, dwi_header);
        else
          grad.load (opt[0][0]);
      }
      else if (dwi_header.DW_scheme().is_set()) {
        grad = dwi_header.DW_scheme();
      }
      else {
        try {
          load_bvecs_bvals (Path::dirname (dwi_header.name()), grad, dwi_header);
        } catch(...) {
          throw Exception ("no diffusion encoding found in image \"" + dwi_header.name() + "\" or corresponding directory");
        }
      }

      if (grad.rows() < 7 || grad.columns() != 4)
        throw Exception ("unexpected diffusion encoding matrix dimensions");

      info ("found " + str (grad.rows()) + "x" + str (grad.columns()) + " diffusion-weighted encoding");

      if (dwi_header.dim (3) != (int) grad.rows())
        throw Exception ("number of studies in base image does not match that in encoding file");

      DWI::normalise_grad (grad);

      return grad;
    }



    template <typename T> void load_bvecs_bvals (const std::string dir_path, Math::Matrix<T>& grad, const Image::Header& dwi_header)
    {
      std::string bvals_path = dir_path + "bvals";
      std::string bvecs_path = dir_path + "bvecs";
      bool found_bvals = Path::is_file (bvals_path);
      bool found_bvecs = Path::is_file (bvecs_path);

      if (!found_bvals && !found_bvecs) {
        const std::string prefix = dwi_header.name().substr (0, dwi_header.name().find_last_of ('.'));
        bvals_path = prefix + "_bvals";
        bvecs_path = prefix + "_bvecs";
        found_bvals = Path::is_file (bvals_path);
        found_bvecs = Path::is_file (bvecs_path);
      }

      if (found_bvals && !found_bvecs)
        throw Exception ("found bvals file but not bvecs file");
      else if (!found_bvals && found_bvecs)
        throw Exception ("found bvecs file but not bvals file");
      else if (!found_bvals && !found_bvecs)
        throw Exception ("could not find either bvecs or bvals gradient files");

      Math::Matrix<T> bvals, bvecs;
      bvals.load (bvals_path);
      bvecs.load (bvecs_path);

      if (bvals.rows() != 1) throw Exception ("bvals file must contain 1 row only");
      if (bvecs.rows() != 3) throw Exception ("bvecs file must contain exactly 3 rows");

      if (bvals.columns() != bvecs.columns() || bvals.columns() != size_t(dwi_header.dim (3)))
        throw Exception ("bvals and bvecs files must have same number of diffusion directions as DW-image");

      const Math::Matrix<float>& M (dwi_header.transform());

      const Point<int> strides (dwi_header.stride (0), dwi_header.stride (1), dwi_header.stride (2));

      Point<int> axis_reorder;
      Point<uint8_t> axis_flip;
      for (size_t axis = 0; axis != 3; ++axis) {
        axis_reorder[Math::abs (strides[axis]) - 1] = axis;
        axis_flip   [Math::abs (strides[axis]) - 1] = (strides[axis] < 0);
      }

      grad.allocate (bvals.columns(), 4);
      for (size_t dir = 0; dir != bvals.columns(); ++dir) {

        const Point<T> init_vector (bvecs (0, dir), bvecs (1, dir), bvecs (2, dir));

        const Point<T> reordered (init_vector[axis_reorder[0]] * (axis_flip[0] ? -1.0 : 1.0),
                                  init_vector[axis_reorder[1]] * (axis_flip[1] ? -1.0 : 1.0),
                                  init_vector[axis_reorder[2]] * (axis_flip[2] ? -1.0 : 1.0));

        const Point<T> rotated (reordered[0]*M(0,0) + reordered[1]*M(0,1) + reordered[2]*M(0,2),
                                reordered[0]*M(1,0) + reordered[1]*M(1,1) + reordered[2]*M(1,2),
                                reordered[0]*M(2,0) + reordered[1]*M(2,1) + reordered[2]*M(2,2));

        grad (dir, 0) = rotated[0];
        grad (dir, 1) = rotated[1];
        grad (dir, 2) = rotated[2];
        grad (dir, 3) = (bvals (0, dir) < 10.0) ? 0.0 : bvals (0, dir);
      }
    }

  }
}

#endif

