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

#include "dwi/directions/assigner.h"

#include "math/rng.h"


namespace MR {
  namespace DWI {
    namespace Directions {



      index_type Assigner::operator() (const Eigen::Vector3d& dir) const
      {
        // TODO The purpose of this operation is to speed up assignment of directions to dixels,
        //   yet it involves expensive trigonometric operations...
        // TODO Alternative would be to construct lookup table in Euclidean space
        Eigen::Vector2d azel;
        Math::Sphere::cartesian2spherical (dir, azel);
        const size_t az_index = std::floor ((azel[0] - az_begin) / az_grid_step);
        const size_t el_index = std::floor ((azel[1] - el_begin) / el_grid_step);
        return (*this) (dir, grid_lookup (az_index, el_index));
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
        const size_t num_az_grids = std::ceil (std::sqrt (2.0 * size()));
        const size_t num_el_grids = std::ceil (std::sqrt (0.5 * size()));
        az_grid_step = 2.0 * Math::pi / default_type(num_az_grids);
        el_grid_step =       Math::pi / default_type(num_el_grids);

        // Grid elements are defined by their lower corners;
        //   this will be reflected in std::floor() operations for allocation of az-el pairs to grid elements
        az_begin = -Math::pi;
        el_begin = 0.0;

        // Rather than storing in a 1D vector, here we're going to construct a 2D matrix
        grid_lookup = Eigen::Array<index_type, Eigen::Dynamic, Eigen::Dynamic>::Constant (num_az_grids, num_el_grids, size());
        for (size_t az_index = 0; az_index != num_az_grids; ++az_index) {
          const default_type az = az_begin + (az_index+0.5)*az_grid_step;
          for (size_t el_index = 0; el_index != num_el_grids; ++el_index) {
            const default_type el = el_begin + (el_index+0.5)*el_grid_step;
            Eigen::Vector3d p;
            Math::Sphere::spherical2cartesian (Eigen::Vector2d({az, el}), p);
            const index_type nearest_dir = (*this) (p, 0);
            grid_lookup (az_index, el_index) = nearest_dir;
          }
        }

        DEBUG ("Lookup table from spherical coordinates to nearest of "
               + str(size()) + "-direction set constructed using "
               + str(num_az_grids) + " x " + str(num_el_grids) + " = " + str(num_az_grids * num_el_grids)
               + " elements, with grid size " + str(az_grid_step) + " x " + str(el_grid_step));
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

