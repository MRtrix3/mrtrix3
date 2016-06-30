/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */




#include "dwi/directions/set.h"

#include <list>
#include <set>

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
          case 5000: az_el_pairs = electrostatic_repulsion_5000 (); return;
          default: throw Exception ("No pre-defined data set of " + str (i) + " directions");
        }
      }



      void Set::initialise_adjacency()
      {
        adj_dirs.assign (size(), std::vector<dir_t>());

        // New algorithm for determining direction adjacency
        // * Duplicate all directions to get a full spherical set
        // * Initialise convex hull using a tetrahedron:
        //   - Identify 6 extremum points (upper & lower per axis)
        //   - Take the two furthest extrema to form a line
        //   - Take the furthest extremum from this line to form a triangle
        //   - Take the furthest vertex from this triangle to form a tetrahedon
        // * For each plane (until none remaining):
        //   * Select the point with the furthest distance from the plane
        //     - If none exists, append these three edges to the result
        //   * Identify any planes adjacent to this plane, for which the point is also above the plane
        //   * Generate new triangles using this point, and the outer border of the planes
        //   * Append these to the list of planes to process

        class Vertex {
          public:
            Vertex (const Set& set, const dir_t index, const bool inverse) :
                dir (set[index] * (inverse ? -1.0f : 1.0f)),
                index (index) { }
            const Eigen::Vector3f dir;
            const dir_t index; // Indexes the underlying direction set
        };

        class Plane {
          public:
            Plane (const std::vector<Vertex>& vertices, const dir_t one, const dir_t two, const dir_t three) :
                indices {{ one, two, three }},
                normal (((vertices[two].dir-vertices[one].dir).cross (vertices[three].dir-vertices[two].dir)).normalized()),
                dist (std::max ( { vertices[one].dir.dot (normal), vertices[two].dir.dot (normal), vertices[three].dir.dot (normal) } ) ) { }
            bool includes (const dir_t i) const { return (indices[0] == i || indices[1] == i || indices[2] == i); }
            const std::array<dir_t,3> indices; // Indexes the vertices vector
            const Eigen::Vector3f normal;
            const float dist;
        };

        class PlaneComp
        {
          public:
            bool operator() (const Plane& one, const Plane& two) const {
              return (one.dist < two.dist);
            }
        };

        std::vector<Vertex> vertices;
        // Generate antipodal vertices
        for (dir_t i = 0; i != size(); ++i) {
          vertices.push_back (Vertex (*this, i, false));
          vertices.push_back (Vertex (*this, i, true));
        }

        dir_t extremum_indices[3][2] = { {0, 0}, {0, 0}, {0, 0} };
        float extremum_values[3][2] = { {1.0f, -1.0f}, {1.0f, -1.0f}, {1.0f, -1.0f} };
        for (size_t i = 0; i != vertices.size(); ++i) {
          for (size_t axis = 0; axis != 3; ++axis) {
            if (vertices[i].dir[axis] < extremum_values[axis][0]) {
              extremum_values[axis][0] = vertices[i].dir[axis];
              extremum_indices[axis][0] = i;
            }
            if (vertices[i].dir[axis] > extremum_values[axis][1]) {
              extremum_values[axis][1] = vertices[i].dir[axis];
              extremum_indices[axis][1] = i;
            }
          }
        }

        // Find the two most distant points out of these six
        std::vector<dir_t> all_extrema;
        for (size_t axis = 0; axis != 3; ++axis) {
          all_extrema.push_back (extremum_indices[axis][0]);
          all_extrema.push_back (extremum_indices[axis][1]);
        }
        std::pair<dir_t, dir_t> distant_pair;
        float max_dist_sq = 0.0f;
        for (dir_t i = 0; i != 6; ++i) {
          for (dir_t j = i + 1; j != 6; ++j) {
            const float dist_sq = (vertices[all_extrema[j]].dir - vertices[all_extrema[i]].dir).squaredNorm();
            if (dist_sq > max_dist_sq) {
              max_dist_sq = dist_sq;
              distant_pair = std::make_pair (i, j);
            }
          }
        }

        // This forms the base line of the base triangle of the tetrahedon
        // Now from the remaining four extrema, find which one is farthest from this line
        dir_t third_point = 6;
        float max_dist = 0.0f;
        for (dir_t i = 0; i != 6; ++i) {
          if (i != distant_pair.first && i != distant_pair.second) {
            const float dist = (vertices[all_extrema[i]].dir - (vertices[all_extrema[distant_pair.first]].dir)).cross (vertices[all_extrema[i]].dir - vertices[all_extrema[distant_pair.second]].dir).norm() / (vertices[all_extrema[distant_pair.second]].dir - vertices[all_extrema[distant_pair.first]].dir).norm();
            if (dist > max_dist) {
              max_dist = dist;
              third_point = i;
            }
          }
        }
        assert (third_point != 6);

        std::multiset<Plane, PlaneComp> planes;
        planes.insert (Plane (vertices, all_extrema[distant_pair.first], all_extrema[distant_pair.second], all_extrema[third_point]));
        // Find the most distant point to this plane, and use it as the tip point of the tetrahedon
        const Plane base_plane = *planes.begin();
        dir_t fourth_point = vertices.size();
        max_dist = 0.0f;
        for (dir_t i = 0; i != vertices.size(); ++i) {
          // Use the reverse of the base plane normal - searching the other hemisphere
          const float dist = vertices[i].dir.dot (-base_plane.normal);
          if (dist > max_dist) {
            max_dist = dist;
            fourth_point = i;
          }
        }
        assert (fourth_point != vertices.size());
        planes.insert (Plane (vertices, base_plane.indices[0], fourth_point, base_plane.indices[1]));
        planes.insert (Plane (vertices, base_plane.indices[1], fourth_point, base_plane.indices[2]));
        planes.insert (Plane (vertices, base_plane.indices[2], fourth_point, base_plane.indices[0]));

        std::vector<Plane> hull;

        // Speedup: Only test those directions that have not yet been incorporated into any plane
        std::list<size_t> unassigned;
        for (size_t i = 0; i != vertices.size(); ++i) {
          if (!base_plane.includes (i) && fourth_point != i)
            unassigned.push_back (i);
        }

        while (planes.size()) {
          Plane current (*planes.begin());
          auto max_index = unassigned.end();
          float max_dist = current.dist;
          for (auto d = unassigned.begin(); d != unassigned.end(); ++d) {
            const float dist = vertices[*d].dir.dot (current.normal);
            if (dist > max_dist) {
              max_dist = dist;
              max_index = d;
            }
          }

          if (max_index == unassigned.end()) {
            hull.push_back (current);
            planes.erase (planes.begin());
          } else {

            // Identify all planes that this extremum point is above
            // More generally this would need to be constrained to only those faces adjacent to the
            //   current plane, but because the data are on the sphere a complete search should be fine
            std::vector<std::multiset<Plane, PlaneComp>::iterator> all_planes;
            for (std::multiset<Plane, PlaneComp>::iterator p = planes.begin(); p != planes.end(); ++p) {
              if (!p->includes (*max_index) && vertices[*max_index].dir.dot (p->normal) > p->dist)
                all_planes.push_back (p);
            }

            // Find the matching edges from multiple faces, and construct new triangles going up to the new point
            // Remove any shared edges; non-shared edges are the projection horizon
            std::set<std::pair<dir_t, dir_t>> horizon;
            for (auto& p : all_planes) {
              for (size_t edge_index = 0; edge_index != 3; ++edge_index) {
                std::pair<dir_t, dir_t> edge;
                switch (edge_index) {
                  case 0: edge = std::make_pair (p->indices[0], p->indices[1]); break;
                  case 1: edge = std::make_pair (p->indices[1], p->indices[2]); break;
                  case 2: edge = std::make_pair (p->indices[2], p->indices[0]); break;
                }
                bool found = false;
                for (auto h = horizon.begin(); h != horizon.end(); ++h) {
                  // Since we are only dealing with triangles, the edge will always be in the opposite
                  //   direction on the adjacent face
                  if (h->first == edge.second && h->second == edge.first) {
                    horizon.erase (h);
                    found = true;
                    break;
                  }
                }
                if (!found)
                  horizon.insert (edge);
              }
            }

            for (auto& h : horizon)
              planes.insert (Plane (vertices, h.first, h.second, *max_index));

            // Delete the used faces
            for (auto i : all_planes)
              planes.erase (i);

            // This point no longer needs to be tested
            unassigned.erase (max_index);

          }
        }

        for (auto& current : hull) {
          // Each of these three directions is adjacent
          // However: Each edge may have already been added from other triangles
          for (size_t edge = 0; edge != 6; ++edge) {
            dir_t from = 0, to = 0;
            switch (edge) {
              case 0: from = vertices[current.indices[0]].index; to = vertices[current.indices[1]].index; break;
              case 1: from = vertices[current.indices[1]].index; to = vertices[current.indices[0]].index; break;
              case 2: from = vertices[current.indices[1]].index; to = vertices[current.indices[2]].index; break;
              case 3: from = vertices[current.indices[2]].index; to = vertices[current.indices[1]].index; break;
              case 4: from = vertices[current.indices[0]].index; to = vertices[current.indices[2]].index; break;
              case 5: from = vertices[current.indices[2]].index; to = vertices[current.indices[0]].index; break;
            }
            bool found = false;
            for (auto i : adj_dirs[from]) {
              if (i == to) {
                found = true;
                break;
              }
            }
            if (!found)
              adj_dirs[from].push_back (to);
          }
        }

        for (auto& i : adj_dirs)
          std::sort (i.begin(), i.end());
      }

      void Set::initialise_mask()
      {
        dir_mask_bytes = (size() + 7) / 8;
        dir_mask_excess_bits = (8 * dir_mask_bytes) - size();
        dir_mask_excess_bits_mask = 0xFF >> dir_mask_excess_bits;
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

        grid_lookup.assign (total_num_angle_grids, std::vector<dir_t>());
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

