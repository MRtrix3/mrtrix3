/* Copyright (c) 2008-2019 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#include "filter/kernels.h"

#include "header.h"

namespace MR
{
  namespace Filter
  {
    namespace Kernels
    {



      kernel_type identity (const ssize_t size)
      {
        assert (size % 2);
        kernel_type result (size);
        const size_t numel = Math::pow3 (size);
        result[(numel-1) / 2] = 1;
        return result;
      }



      kernel_type boxblur (const ssize_t size)
      {
        assert (size % 2);
        return boxblur ({int(size), int(size), int(size)});
      }
      kernel_type boxblur (const vector<int>& sizes)
      {
        assert (sizes.size() == 3);
#ifndef NDEBUG
        for (auto i : sizes)
          assert (i % 2);
#endif
        kernel_type result (sizes);
        const default_type value = 1.0 / default_type(result.size());
        result.fill (value);
        return result;
      }



      kernel_type gaussian (const Header& header, const default_type fwhm, const default_type radius)
      {
        assert (fwhm >= 0.0);
        assert (radius >= 0.0);
        // Initialise kernel fullwidths
        //   As soon as voxel distance _exceeds_ radius, weight is set to zero
        kernel_type result ( vector<int> { 1 + 2*int(std::floor (radius / header.spacing(0))),
                                           1 + 2*int(std::floor (radius / header.spacing(1))),
                                           1 + 2*int(std::floor (radius / header.spacing(2))) } );
        Eigen::Matrix<int, 3, 1> offset;
        default_type mass = 0.0;
        const default_type sq_radius = Math::pow2 (radius);
        const default_type sigma = fwhm / (2.0 * std::sqrt (2.0 * std::log (2.0)));
        const default_type sq_sigma = Math::pow2 (sigma);
        size_t index = 0;
        for (offset[2] = -int(result.halfsize (2)); offset[2] <= int(result.halfsize (2)); ++offset[2]) {
          for (offset[1] = -int(result.halfsize (1)); offset[1] <= int(result.halfsize (1)); ++offset[1]) {
            for (offset[0] = -int(result.halfsize (0)); offset[0] <= int(result.halfsize (0)); ++offset[0], ++index) {
              const default_type sq_distance = offset.squaredNorm();
              if (sq_distance <= sq_radius) {
                const default_type weight = std::exp (-sq_distance / sq_sigma);
                result[index] = weight;
                mass += weight;
              } else {
                result[index] = 0.0;
              }
            }
          }
        }
        result *= (1.0 / mass);
        return result;
      }



      kernel_type laplacian3d()
      {
        const default_type m = 1.0/26.0;
        kernel_type result (3);
        result << 2.0*m,   3.0*m, 2.0*m,
                  3.0*m,   6.0*m, 3.0*m,
                  2.0*m,   3.0*m, 2.0*m,

                  3.0*m,   6.0*m, 3.0*m,
                  6.0*m, -88.0*m, 6.0*m,
                  3.0*m,   6.0*m, 3.0*m,

                  2.0*m,   3.0*m, 2.0*m,
                  3.0*m,   6.0*m, 3.0*m,
                  2.0*m,   3.0*m, 2.0*m;
        return result;
      }



      kernel_type radialblur (const Header& header, const default_type radius)
      {
        assert (radius >= 0.0);
        const vector<int> half_extents ({
            int(std::floor (radius / header.spacing(0))),
            int(std::floor (radius / header.spacing(1))),
            int(std::floor (radius / header.spacing(2))) });
        const vector<int> extents ({
            1 + (2*half_extents[0]),
            1 + (2*half_extents[1]),
            1 + (2*half_extents[2]) });
        const float sqradius = Math::pow2 (radius);
        kernel_type result (extents);
        size_t voxel_count = 0;
        size_t index = 0;
        for (int z = -half_extents[2]; z <= half_extents[2]; ++z) {
          for (int y = -half_extents[1]; y <= half_extents[1]; ++y) {
            for (int x = -half_extents[0]; x <= half_extents[0]; ++x, ++index) {
              const bool inside = Math::pow2 (x) + Math::pow2 (y) + Math::pow2 (z) <= sqradius;
              result[index] = inside ? 1.0 : 0.0;
              if (inside)
                ++voxel_count;
            }
          }
        }
        result *= 1.0 / default_type (voxel_count);
        return result;
      }



      kernel_type sharpen (const default_type strength)
      {
        assert (strength >= 0.0);
        kernel_type result (3);
        result.fill (0.0);
        result[13] = 1.0 + (6.0 * strength);
        result[4] = result[10] = result[12] = result[14] = result[16] = result[22] = -strength;
        return result;
      }



      kernel_type unsharp_mask (const Header& header, const default_type smooth_fwhm, const default_type sharpen_strength)
      {
        // Initial smoothing kernel
        kernel_type result (gaussian (header, smooth_fwhm, 3.0 * smooth_fwhm));
        // Subtract this from the original image to get the unsharp mask
        result *= -1.0;
        const size_t central_voxel_index = (result.size() - 1)/2;
        result[central_voxel_index] += 1.0;
        // Now take the original image, and add some fraction of the unsharp mask
        result *= sharpen_strength;
        result[central_voxel_index] += 1.0;
        return result;
      }








      namespace
      {
        kernel_type::base_type aTb (const kernel_type::base_type& a, const kernel_type::base_type& b)
        {
          kernel_type::base_type result (a.size() * b.size());
          ssize_t counter = 0;
          for (ssize_t ib = 0; ib != b.size(); ++ib) {
            for (ssize_t ia = 0; ia != a.size(); ++ia)
              result[counter++] = a[ia] * b[ib];
          }
          return result;
        }

        kernel_triplet make_triplet (const kernel_type::base_type& prefilter, const kernel_type::base_type derivative)
        {
          assert (prefilter.size() == derivative.size());
          return { aTb (aTb (derivative, prefilter), prefilter),
                   aTb (aTb (prefilter, derivative), prefilter),
                   aTb (aTb (prefilter, prefilter), derivative) };
        }
      }




      kernel_triplet sobel()
      {
        kernel_type::base_type triangle (3);
        triangle << 0.25, 0.50, 0.25;
        kernel_type::base_type edge (3);
        edge << -1.00, 0.00, 1.00;

        return make_triplet (triangle, edge);
      }



      kernel_triplet sobel_feldman()
      {
        kernel_type::base_type triangle (3);
        triangle << 3.0/16.0, 10.0/16.0, 3.0/16.0;
        kernel_type::base_type edge (3);
        edge << -1.00, 0.00, 1.00;

        return make_triplet (triangle, edge);
      }



      kernel_triplet farid (const size_t order, const size_t size)
      {
        assert (order > 0);

        if (!(size % 2))
          throw Exception ("Farid derivative kernel extent must be odd");
        if (order > (size-1) / 2)
          throw Exception ("Farid derivative order " + str(order) + " not possible with kernel size " + str(size));
        if (order > 3)
          throw Exception ("Farid derivatives not available for orders greater than 3");

        kernel_type::base_type prefilter (size), derivative (size);
        switch (size) {
          case 3:
            prefilter  <<  0.229789,  0.540242,  0.229789;
            derivative << -0.425287,  0.000000,  0.425287;
            break;
          case 5:
            switch (order) {
              case 1:
                prefilter  <<  0.037659,  0.249153,  0.426375,  0.249153,  0.037659;
                derivative << -0.109604, -0.276691,  0.000000,  0.276691,  0.109604;
                break;
              case 2:
                prefilter  <<  0.030320,  0.249724,  0.439911,  0.249724,  0.030320;
                derivative <<  0.232905,  0.002668, -0.471147,  0.002668,  0.232905;
                break;
            }
            break;
          case 7:
            if (order <= 2)
              prefilter << 0.004711, 0.069321, 0.245410, 0.361117, 0.245410, 0.069321, 0.004711;
            else
              prefilter << 0.003992, 0.067088, 0.246217, 0.365406, 0.246217, 0.067088, 0.003992;
            switch (order) {
              case 1:
                derivative << -0.018708, -0.125376, -0.193091,  0.000000,  0.193091,  0.125376,  0.018708;
                break;
              case 2:
                derivative <<  0.055336,  0.137778, -0.056554, -0.273118, -0.056554,  0.137778,  0.055336;
                break;
              case 3:
                derivative << -0.111680,  0.012759,  0.336539,  0.000000, -0.336539, -0.012759,  0.111680;
                break;
              default: assert (0);
            }
            break;
          case 9:
            prefilter << 0.000721, 0.015486, 0.090341, 0.234494, 0.317916, 0.234494, 0.090341, 0.015486, 0.000721;
            switch (order) {
              case 1:
                derivative << -0.003059, -0.035187, -0.118739, -0.143928,  0.000000,  0.143928,  0.118739,  0.035187,  0.003059;
                break;
              case 2:
                derivative <<  0.010257,  0.061793,  0.085598, -0.061661, -0.191974, -0.061661,  0.085598,  0.061793,  0.010257;
                break;
              case 3:
                derivative << -0.027205, -0.065929,  0.053614,  0.203718,  0.000000, -0.203718, -0.053614,  0.065929,  0.027205;
                break;
              default:
                assert (0);
            }
            break;
          default:
            throw Exception ("Farid kernel only supported up to kernel size 9");
        }

        return make_triplet (prefilter, derivative);
      }



    }
  }
}
