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

#include "dwi/directions/set.h"

#include "math/rng.h"


namespace MR {
  namespace DWI {
    namespace Directions {



      index_type FastLookupSet::operator() (const Eigen::Vector3d& p) const
      {

        const size_t grid_index = dir2gridindex (p);

        index_type best_dir = grid_lookup[grid_index].front();
        default_type max_dp = abs (p.dot ((*this)[best_dir]));
        for (size_t i = 1; i != grid_lookup[grid_index].size(); ++i) {
          const index_type this_dir = (grid_lookup[grid_index])[i];
          const default_type this_dp = abs (p.dot ((*this) [this_dir]));
          if (this_dp > max_dp) {
            max_dp = this_dp;
            best_dir = this_dir;
          }
        }

        return best_dir;

      }



      index_type FastLookupSet::nearest_exhaustive (const Eigen::Vector3d& p) const
      {

        index_type dir = 0;
        default_type max_dot_product = abs (p.dot ((*this)[0]));
        for (size_t i = 1; i != size(); ++i) {
          const default_type this_dot_product = abs (p.dot ((*this)[i]));
          if (this_dot_product > max_dot_product) {
            max_dot_product = this_dot_product;
            dir = i;
          }
        }
        return dir;

      }




      void FastLookupSet::initialise()
      {

        default_type adj_dot_product_sum = 0.0;
        size_t adj_dot_product_count = 0;
        for (size_t i = 0; i != size(); ++i) {
          for (const auto j : adjacency[i]) {
            if (j > i) {
              adj_dot_product_sum += abs ((*this)[i].dot ((*this)[j]));
              ++adj_dot_product_count;
            }
          }
        }

        const default_type min_dp = adj_dot_product_sum / default_type(adj_dot_product_count);
        const default_type max_angle_step = acos (min_dp);

        num_az_grids = ceil (2.0 * Math::pi / max_angle_step);
        num_el_grids = ceil (      Math::pi / max_angle_step);
        total_num_angle_grids = num_az_grids * num_el_grids;

        az_grid_step = 2.0 * Math::pi / default_type(num_az_grids - 1);
        el_grid_step =       Math::pi / default_type(num_el_grids - 1);

        az_begin = -Math::pi;
        el_begin = 0.0;

        grid_lookup.assign (total_num_angle_grids, vector<index_type>());
        for (size_t i = 0; i != size(); ++i) {
          const size_t grid_index = dir2gridindex ((*this)[i]);
          grid_lookup[grid_index].push_back (i);
        }

        for (size_t i = 0; i != total_num_angle_grids; ++i) {

          const size_t az_index = i / num_el_grids;
          const size_t el_index = i - (az_index * num_el_grids);

          for (size_t point_index = 0; point_index != 4; ++point_index) {

            default_type az = az_begin + (az_index * az_grid_step);
            default_type el = el_begin + (el_index * el_grid_step);
            switch (point_index) {
              case 0: break;
              case 1: az += az_grid_step; break;
              case 2: az += az_grid_step; el += el_grid_step; break;
              case 3: el += el_grid_step; break;
            }

            Eigen::Vector3d p;
            Math::Sphere::spherical2cartesian (Eigen::Vector2d({az, el}), p);
            const index_type nearest_dir = nearest_exhaustive (p);
            bool dir_present = false;
            for (vector<index_type>::const_iterator d = grid_lookup[i].begin(); !dir_present && d != grid_lookup[i].end(); ++d)
              dir_present = (*d == nearest_dir);
            if (!dir_present)
              grid_lookup[i].push_back (nearest_dir);

          }

        }

        for (size_t grid_index = 0; grid_index != total_num_angle_grids; ++grid_index) {
          vector<index_type>& this_grid (grid_lookup[grid_index]);
          const size_t num_to_expand = this_grid.size();
          for (size_t index_to_expand = 0; index_to_expand != num_to_expand; ++index_to_expand) {
            const index_type dir_to_expand = this_grid[index_to_expand];
            for (vector<index_type>::const_iterator adj = adjacency[dir_to_expand].begin(); adj != adjacency[dir_to_expand].end(); ++adj) {
              // Size of lookup tables could potentially be reduced by being more prohibitive of adjacent direction inclusion in the lookup table for this grid
              bool is_present = false;
              for (vector<index_type>::const_iterator i = this_grid.begin(); !is_present && i != this_grid.end(); ++i)
                is_present = (*i == *adj);
              if (!is_present)
                this_grid.push_back (*adj);

            }
          }
          std::sort (this_grid.begin(), this_grid.end());
        }

      }



      size_t FastLookupSet::dir2gridindex (const Eigen::Vector3d& p) const
      {

        const default_type azimuth   = atan2(p[1], p[0]);
        const default_type elevation = acos (p[2]);

        const size_t azimuth_grid   = std::floor (( azimuth  - az_begin) / az_grid_step);
        const size_t elevation_grid = std::floor ((elevation - el_begin) / el_grid_step);
        const size_t index = (azimuth_grid * num_el_grids) + elevation_grid;

        return index;

      }



      void FastLookupSet::test_lookup() const
      {
        Math::RNG rng;
        std::normal_distribution<> normal (0.0, 1.0);

        size_t error_count = 0;
        const size_t checks = 1000000;
        for (size_t i = 0; i != checks; ++i) {
          Eigen::Vector3d p (normal(rng), normal(rng), normal(rng));
          p.normalize();
          if ((*this) (p) != nearest_exhaustive (p))
            ++error_count;
        }
        const default_type error_rate = default_type(error_count) / default_type(checks);
        VAR (error_rate);
      }




    }
  }
}

