/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by Robert Smith, 2011.

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




#include "dwi/directions/set.h"

#include "math/rng.h"


namespace MR {
  namespace DWI {
    namespace Directions {





      dir_t Set::get_min_linkage (const dir_t one, const dir_t two) const
      {
        if (one == two)
          return 0;

        std::vector<bool> processed (size(), 0);
        std::vector<dir_t> to_expand;
        processed[one] = true;
        to_expand.push_back (one);
        dir_t min_linkage = 0;
        do {
          ++min_linkage;
          std::vector<dir_t> next_to_expand;
          for (const auto& i : to_expand) {
            for (const auto& j : adj_dirs[i]) {
              if (j == two) {
                return min_linkage;
              } else if (!processed[j]) {
                processed[j] = true;
                next_to_expand.push_back (j);
              }
            }
          }
          std::swap (to_expand, next_to_expand);
        } while (1);
        return std::numeric_limits<dir_t>::max();
      }



      void Set::load_predefined (Eigen::MatrixXd& az_el_pairs, const size_t i)
      {
        switch (i) {
          case 60:   az_el_pairs = electrostatic_repulsion_60 (); return;
          case 129:  az_el_pairs = tesselation_129 (); return;
          case 300:  az_el_pairs = electrostatic_repulsion_300 (); return;
          case 321:  az_el_pairs = tesselation_321 (); return;
          case 469:  az_el_pairs = tesselation_469 (); return;
          case 513:  az_el_pairs = tesselation_513 (); return;
          case 1281: az_el_pairs = tesselation_1281 (); return;
          default: throw Exception ("No pre-defined data set of " + str (i) + " directions!");
        }
      }



      void Set::initialise (const Eigen::MatrixXd& az_el_pairs)
      {
        unit_vectors.resize (az_el_pairs.rows());
        for (size_t i = 0; i != size(); ++i) {
          const float azimuth   = az_el_pairs(i, 0);
          const float elevation = az_el_pairs(i, 1);
          const float sin_elevation = std::sin (elevation);
          unit_vectors[i] = { std::cos (azimuth) * sin_elevation, std::sin (azimuth) * sin_elevation, std::cos (elevation) };
        }

        adj_dirs = new std::vector<dir_t> [size()];
        for (dir_t i = 0; i != size(); ++i) {
          for (dir_t j = 0; j != size(); ++j) {
            if (j != i) {

              Eigen::Vector3f p;
              if (unit_vectors[i].dot (unit_vectors[j]) > 0.0)
                p = ((unit_vectors[i] + unit_vectors[j]).normalized());
              else
                p = ((unit_vectors[i] - unit_vectors[j]).normalized());
              const float dot_to_i = std::abs (p.dot (unit_vectors[i]));
              const float dot_to_j = std::abs (p.dot (unit_vectors[j]));
              const float this_dot_product = std::max (dot_to_i, dot_to_j);

              bool is_adjacent = true;
              for (dir_t k = 0; (k != size()) && is_adjacent; ++k) {
                if ((k != i) && (k != j)) {
                  if (fabs (p.dot (unit_vectors[k])) > this_dot_product)
                    is_adjacent = false;
                }
              }

              if (is_adjacent)
                adj_dirs[i].push_back (j);

            }
          }
        }

        dir_mask_bytes = (size() + 7) / 8;
        dir_mask_excess_bits = (8 * dir_mask_bytes) - size();
        dir_mask_excess_bits_mask = 0xFF >> dir_mask_excess_bits;

      }



      Set::~Set()
      {
        if (adj_dirs) {
          delete[] adj_dirs;
          adj_dirs = NULL;
        }
      }







      FastLookupSet::FastLookupSet (const FastLookupSet& that) :
        Set (that),
        grid_lookup (new std::vector<dir_t>[that.total_num_angle_grids]),
        num_az_grids (that.num_az_grids),
        num_el_grids (that.num_el_grids),
        total_num_angle_grids (that.total_num_angle_grids),
        az_grid_step (that.az_grid_step),
        el_grid_step (that.el_grid_step),
        az_begin (that.az_begin),
        el_begin (that.el_begin)
      {
        for (size_t i = 0; i != total_num_angle_grids; ++i)
          grid_lookup[i] = that.grid_lookup[i];
      }



      FastLookupSet::~FastLookupSet ()
      {
        if (grid_lookup) {
          delete[] grid_lookup;
          grid_lookup = NULL;
        }
      }



      dir_t FastLookupSet::select_direction (const Eigen::Vector3f& p) const
      {

        const size_t grid_index = dir2gridindex (p);

        dir_t best_dir = grid_lookup[grid_index].front();
        float max_dp = std::abs (p.dot (get_dir (best_dir)));
        for (size_t i = 1; i != grid_lookup[grid_index].size(); ++i) {
          const dir_t this_dir = (grid_lookup[grid_index])[i];
          const float this_dp = std::abs (p.dot (get_dir (this_dir)));
          if (this_dp > max_dp) {
            max_dp = this_dp;
            best_dir = this_dir;
          }
        }

        return best_dir;

      }



      dir_t FastLookupSet::select_direction_slow (const Eigen::Vector3f& p) const
      {

        dir_t dir = 0;
        float max_dot_product = std::abs (p.dot (unit_vectors[0]));
        for (size_t i = 1; i != size(); ++i) {
          const float this_dot_product = std::abs (p.dot (unit_vectors[i]));
          if (this_dot_product > max_dot_product) {
            max_dot_product = this_dot_product;
            dir = i;
          }
        }
        return dir;

      }




      void FastLookupSet::initialise()
      {

        double adj_dot_product_sum = 0.0;
        size_t adj_dot_product_count = 0;
        for (size_t i = 0; i != size(); ++i) {
          for (std::vector<dir_t>::const_iterator j = adj_dirs[i].begin(); j != adj_dirs[i].end(); ++j) {
            if (*j > i) {
              adj_dot_product_sum += std::abs (unit_vectors[i].dot (unit_vectors[*j]));
              ++adj_dot_product_count;
            }
          }
        }

        const float min_dp = adj_dot_product_sum / double(adj_dot_product_count);
        const float max_angle_step = acos (min_dp);

        num_az_grids = ceil (2.0 * Math::pi / max_angle_step);
        num_el_grids = ceil (      Math::pi / max_angle_step);
        total_num_angle_grids = num_az_grids * num_el_grids;

        az_grid_step = 2.0 * Math::pi / float(num_az_grids - 1);
        el_grid_step =       Math::pi / float(num_el_grids - 1);

        az_begin = -Math::pi;
        el_begin = 0.0;

        grid_lookup = new std::vector<dir_t>[total_num_angle_grids];
        for (size_t i = 0; i != size(); ++i) {
          const size_t grid_index = dir2gridindex (get_dir(i));
          grid_lookup[grid_index].push_back (i);
        }

        for (size_t i = 0; i != total_num_angle_grids; ++i) {

          const size_t az_index = i / num_el_grids;
          const size_t el_index = i - (az_index * num_el_grids);

          for (size_t point_index = 0; point_index != 4; ++point_index) {

            float az = az_begin + (az_index * az_grid_step);
            float el = el_begin + (el_index * el_grid_step);
            switch (point_index) {
              case 0: break;
              case 1: az += az_grid_step; break;
              case 2: az += az_grid_step; el += el_grid_step; break;
              case 3: el += el_grid_step; break;
            }

            const Eigen::Vector3f p (cos(az) * sin(el), sin(az) * sin(el), cos (el));
            const dir_t nearest_dir = select_direction_slow (p);
            bool dir_present = false;
            for (std::vector<dir_t>::const_iterator d = grid_lookup[i].begin(); !dir_present && d != grid_lookup[i].end(); ++d)
              dir_present = (*d == nearest_dir);
            if (!dir_present)
              grid_lookup[i].push_back (nearest_dir);

          }

        }

        for (size_t grid_index = 0; grid_index != total_num_angle_grids; ++grid_index) {
          std::vector<dir_t>& this_grid (grid_lookup[grid_index]);
          const size_t num_to_expand = this_grid.size();
          for (size_t index_to_expand = 0; index_to_expand != num_to_expand; ++index_to_expand) {
            const dir_t dir_to_expand = this_grid[index_to_expand];
            for (std::vector<dir_t>::const_iterator adj = get_adj_dirs(dir_to_expand).begin(); adj != get_adj_dirs(dir_to_expand).end(); ++adj) {

              // Size of lookup tables could potentially be reduced by being more prohibitive of adjacent direction inclusion in the lookup table for this grid

              bool is_present = false;
              for (std::vector<dir_t>::const_iterator i = this_grid.begin(); !is_present && i != this_grid.end(); ++i)
                is_present = (*i == *adj);
              if (!is_present)
                this_grid.push_back (*adj);

            }
          }
          std::sort (this_grid.begin(), this_grid.end());
        }

      }



      size_t FastLookupSet::dir2gridindex (const Eigen::Vector3f& p) const
      {

        const float azimuth   = atan2(p[1], p[0]);
        const float elevation = acos (p[2]);

        const size_t azimuth_grid   = std::floor (( azimuth  - az_begin) / az_grid_step);
        const size_t elevation_grid = std::floor ((elevation - el_begin) / el_grid_step);
        const size_t index = (azimuth_grid * num_el_grids) + elevation_grid;

        return index;

      }



      void FastLookupSet::test_lookup() const
      {
        Math::RNG rng;
        std::normal_distribution<float> normal (0.0, 1.0);

        size_t error_count = 0;
        const size_t checks = 1000000;
        for (size_t i = 0; i != checks; ++i) {
          Eigen::Vector3f p (normal(rng), normal(rng), normal(rng));
          p.normalize();
          if (select_direction (p) != select_direction_slow (p))
            ++error_count;
        }
        const float error_rate = float(error_count) / float(checks);
        VAR (error_rate);

      }




    }
  }
}

