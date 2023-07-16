/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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

#include "math/sphere/set/assigner.h"

#include "math/rng.h"
#include "math/sphere/sphere.h"


namespace MR {
  namespace Math {
    namespace Sphere {
      namespace Set {



      index_type Assigner::operator() (const Eigen::Vector3d& dir) const
      {
        auto clamp = [&] (const default_type value) -> ssize_t {
          return std::min (ssize_t(resolution-1), std::max (ssize_t(0), ssize_t(value)));
        };
        const Eigen::Array<ssize_t, 3, 1> indices ({
          clamp (std::floor (0.5 * (dir[0] + default_type(1.0)) * resolution)),
          clamp (std::floor (0.5 * (dir[1] + default_type(1.0)) * resolution)),
          clamp (std::floor (0.5 * (dir[2] + default_type(1.0)) * resolution))
        });
        const size_t lookup_index = indices[2]*Math::pow2(resolution) + indices[1]*resolution + indices[0];
        const size_t guess = lookup[lookup_index];
        assert (guess != rows());
        return (*this) (dir, guess);
      }



      index_type Assigner::operator() (const Eigen::Vector3d& dir, const index_type guess) const
      {
        index_type result (guess), previous (guess);
        default_type max_dot_product = abs (dir.dot ((*this)[guess]));
        do {
          previous = result;
          for (const auto& i : adjacency[previous]) {
            const default_type this_dot_product = abs (dir.dot ((*this)[i]));
            if (this_dot_product > max_dot_product) {
              result = i;
              max_dot_product = this_dot_product;
            }
          }
        } while (result != previous);
        return result;
      }



      void Assigner::initialise()
      {
        // Decide on an appropriate resolution of the grid
        //    Seems reasonable that for direction sets with more directions, we want a higher resolution grid
        //    Also not clear that there's much penalty to generating a much higher resolution grid:
        //    calculations are much the same, it's just extra storage / not quite as good caching
        //    Also bear in mind we're dealing with antipodal symmetry for each direction
        //    (we could arbitrarily choose just one hemisphere I suppose?)
        //    Also make it an even number just so that it's symmetric about the origin
        resolution = std::ceil (std::cbrt(2 * rows())/2.0) * 2;
        lookup.assign (Math::pow3 (resolution), rows());
        // Want to know the distance from the centre of the voxel to one of the vertices
        // The total width of each voxel is (2.0 / resolution),
        //   since we're spanning [-1.0, +1.0] on each axis
        // The half-width along one axis is therefore (1.0 / resolution)
        const default_type half_voxel_diagonal (std::sqrt (3.0 * Math::pow2 (1.0 / resolution)));
        size_t fill_count = 0;
        size_t lookup_index = 0;
        for (size_t z_index = 0; z_index != resolution; ++z_index) {
          const default_type z (-1.0 + (z_index + 0.5) * (2.0 / default_type(resolution)));
          for (size_t y_index = 0; y_index != resolution; ++y_index) {
            const default_type y (-1.0 + (y_index + 0.5) * (2.0 / default_type(resolution)));
            for (size_t x_index = 0; x_index != resolution; ++x_index) {
              const default_type x (-1.0 + (x_index + 0.5) * (2.0 / default_type(resolution)));
              Eigen::Vector3d xyz (x, y, z);
              // Is it plausible that a unit vector direction could lie anywhere within this voxel?
              // If it is not, then we don't want to populate the lookup grid element
              if (std::abs (xyz.norm() - default_type(1.0)) < half_voxel_diagonal) {
                xyz.normalize();
                const index_type nearest_dir = (*this) (xyz, 0);
                lookup[lookup_index] = nearest_dir;
                ++fill_count;
              }
              ++lookup_index;
            }
          }
        }
        DEBUG ("Math::Sphere::Set::Assigner for " + str(rows()) + "-direction set initialised"
               + " using a resolution of " + str(resolution)
               + " for a grid of " + str(lookup.size()) + " voxels"
               + " with " + str(fill_count) + " filled elements");
        //test();
      }



      void Assigner::test() const
      {
        Math::RNG rng;
        std::normal_distribution<> normal (0.0, 1.0);

        auto exhaustive = [&] (const Eigen::Vector3d& dir) -> index_type
        {
          index_type result = 0;
          default_type max_dot_product = abs (dir.dot ((*this)[result]));
          for (index_type i = 1; i != size(); ++i) {
            const default_type this_dot_product = abs (dir.dot ((*this)[i]));
            if (this_dot_product > max_dot_product) {
              max_dot_product = this_dot_product;
              result = i;
            }
          }
          return result;
        };

        size_t error_count = 0;
        const size_t checks = 1000000;
        for (size_t i = 0; i != checks; ++i) {
          Eigen::Vector3d p (normal(rng), normal(rng), normal(rng));
          p.normalize();
          if ((*this) (p) != exhaustive (p))
            ++error_count;
        }
        const default_type error_rate = default_type(error_count) / default_type(checks);
        VAR (error_rate);
      }



      }
    }
  }
}

