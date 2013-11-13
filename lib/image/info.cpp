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

#include "image/stride.h"
#include "image/info.h"
#include "image/transform.h"
#include "math/permutation.h"

namespace MR
{
  namespace Image
  {

    namespace
    {

      inline size_t not_any_of (size_t a, size_t b)
      {
        for (size_t i = 0; i < 3; ++i) {
          if (a == i || b == i)
            continue;
          return i;
        }
        assert (0);
        return UINT_MAX;
      }

      void disambiguate_permutation (Math::Permutation& permutation)
      {
        if (permutation[0] == permutation[1])
          permutation[1] = not_any_of (permutation[0], permutation[2]);

        if (permutation[0] == permutation[2])
          permutation[2] = not_any_of (permutation[0], permutation[1]);

        if (permutation[1] == permutation[2])
          permutation[2] = not_any_of (permutation[0], permutation[1]);
      }

    }



    void Info::sanitise_voxel_sizes ()
    {
      if (ndim() < 3) {
        INFO ("image contains fewer than 3 dimensions - adding extra dimensions");
        set_ndim (3);
      }

      if (!finite (vox (0)) || !finite (vox (1)) || !finite (vox (2))) {
        FAIL ("invalid voxel sizes - resetting to sane defaults");
        float mean_vox_size = 0.0;
        size_t num_valid_vox = 0;
        for (size_t i = 0; i < 3; ++i) {
          if (finite(vox(i))) {
            ++num_valid_vox; 
            mean_vox_size += vox(i);
          }
        }
        mean_vox_size /= num_valid_vox;
        for (size_t i = 0; i < 3; ++i) 
          if (!finite(vox(i))) 
            vox(i) = mean_vox_size;
      }
    }


    void Info::sanitise_transform ()
    {
      if (transform().is_set()) {
        if (transform().rows() != 4 || transform().columns() != 4) {
          transform_.clear();
          FAIL ("transform matrix is not 4x4 - resetting to sane defaults");
        }
        else {
          for (size_t i = 0; i < 3; i++) {
            for (size_t j = 0; j < 4; j++) {
              if (!finite (transform_ (i,j))) {
                transform_.clear();
                FAIL ("transform matrix contains invalid entries - resetting to sane defaults");
                break;
              }
            }
            if (!transform().is_set()) break;
          }
        }
      }

      if (!transform().is_set())
        Image::Transform::set_default (transform_, *this);

      transform_ (3,0) = transform_ (3,1) = transform_ (3,2) = 0.0;
      transform_ (3,3) = 1.0;
    }




    void Info::sanitise_strides ()
    {
      Image::Stride::sanitise (*this);
      Image::Stride::symbolise (*this);

      // find which row of the transform is closest to each scanner axis:
      Math::Permutation perm (3);
      Math::absmax (transform().row (0).sub (0,3), perm[0]);
      Math::absmax (transform().row (1).sub (0,3), perm[1]);
      Math::absmax (transform().row (2).sub (0,3), perm[2]);
      disambiguate_permutation (perm);
      assert (perm[0] != perm[1] && perm[1] != perm[2] && perm[2] != perm[0]);

      // figure out whether any of the rows of the transform point in the
      // opposite direction to the MRtrix convention:
      bool flip [3];
      flip[perm[0]] = transform_ (0,perm[0]) < 0.0;
      flip[perm[1]] = transform_ (1,perm[1]) < 0.0;
      flip[perm[2]] = transform_ (2,perm[2]) < 0.0;

      // check if image is already near-axial, return if true:
      if (perm[0] == 0 && perm[1] == 1 && perm[2] == 2 &&
          !flip[0] && !flip[1] && !flip[2])
        return;

      Math::Matrix<float> M (transform());
      Math::Vector<float> translation = M.column (3).sub (0,3);

      // modify translation vector:
      for (size_t i = 0; i < 3; ++i) {
        if (flip[i]) {
          const float length = (dim (i)-1) * vox (i);
          Math::Vector<float> axis = M.column (i);
          for (size_t n = 0; n < 3; ++n) {
            axis[n] = -axis[n];
            translation[n] -= length*axis[n];
          }
        }
      }

      // switch and/or invert rows if needed:
      for (size_t i = 0; i < 3; ++i) {
        Math::Vector<float> row = M.row (i).sub (0,3);
        perm.apply (row);
        if (flip[i])
          stride(i) = -stride(i);
      }

      // copy back transform:
      transform().swap (M);

      // switch axes to match:
      Axis a[] = {
        axes_[perm[0]],
        axes_[perm[1]],
        axes_[perm[2]]
      };
      axes_[0] = a[0];
      axes_[1] = a[1];
      axes_[2] = a[2];

    }


  }
}

