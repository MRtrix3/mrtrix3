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

#include "math/sphere/set/adjacency.h"

#include <list>
#include <set>

#include "misc/bitset.h"


namespace MR {
  namespace Math {
    namespace Sphere {
      namespace Set {



      index_type Adjacency::distance (const index_type one, const index_type two) const
      {
        assert (one < size());
        assert (two < size());
        if (one == two)
          return 0;

        vector<bool> processed (size(), 0);
        vector<index_type> to_expand;
        processed[one] = true;
        to_expand.push_back (one);
        index_type min_linkage = 0;
        do {
          ++min_linkage;
          vector<index_type> next_to_expand;
          for (const auto& i : to_expand) {
            for (const auto& j : (*this)[i]) {
              if (j == two) {
                return min_linkage;
              } else if (!processed[j]) {
                processed[j] = true;
                next_to_expand.push_back (j);
              }
            }
          }
          std::swap (to_expand, next_to_expand);
        } while (true);
        assert (false);
        return std::numeric_limits<index_type>::max();
      }



      void Adjacency::initialise (const Eigen::Matrix<default_type, Eigen::Dynamic, 3>& dirs)
      {
        assign (dirs.rows(), vector<index_type>());

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
            Vertex (const Eigen::Matrix<default_type, Eigen::Dynamic, 3>& dirs, const index_type index, const bool inverse) :
                dir (dirs.row(index) * (inverse ? -1.0 : 1.0)),
                index (index) { }
            const Eigen::Vector3d dir;
            const index_type index; // Indexes the underlying direction set
        };

        class Plane {
          public:
            Plane (const vector<Vertex>& vertices, const index_type one, const index_type two, const index_type three) :
                indices {{ one, two, three }},
                normal (((vertices[two].dir-vertices[one].dir).cross (vertices[three].dir-vertices[two].dir)).normalized()),
                dist (std::max ( { vertices[one].dir.dot (normal), vertices[two].dir.dot (normal), vertices[three].dir.dot (normal) } ) ) { }
            bool includes (const index_type i) const { return (indices[0] == i || indices[1] == i || indices[2] == i); }
            const std::array<index_type,3> indices; // Indexes the vertices vector
            const Eigen::Vector3d normal;
            const default_type dist;
        };

        class PlaneComp {
          public:
            bool operator() (const Plane& one, const Plane& two) const {
              return (one.dist < two.dist);
            }
        };

        vector<Vertex> vertices;
        // Generate antipodal vertices
        for (index_type i = 0; i != size(); ++i) {
          vertices.push_back (Vertex (dirs, i, false));
          vertices.push_back (Vertex (dirs, i, true));
        }

        index_type extremum_indices[3][2] = { {0, 0}, {0, 0}, {0, 0} };
        default_type extremum_values[3][2] = { {1.0, -1.0}, {1.0, -1.0}, {1.0, -1.0} };
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
        vector<index_type> all_extrema;
        for (size_t axis = 0; axis != 3; ++axis) {
          all_extrema.push_back (extremum_indices[axis][0]);
          all_extrema.push_back (extremum_indices[axis][1]);
        }
        std::pair<index_type, index_type> distant_pair;
        default_type max_dist_sq = 0.0;
        for (index_type i = 0; i != 6; ++i) {
          for (index_type j = i + 1; j != 6; ++j) {
            const default_type dist_sq = (vertices[all_extrema[j]].dir - vertices[all_extrema[i]].dir).squaredNorm();
            if (dist_sq > max_dist_sq) {
              max_dist_sq = dist_sq;
              distant_pair = std::make_pair (i, j);
            }
          }
        }

        // This forms the base line of the base triangle of the tetrahedon
        // Now from the remaining four extrema, find which one is farthest from this line
        index_type third_point = 6;
        default_type max_dist = 0.0;
        for (index_type i = 0; i != 6; ++i) {
          if (i != distant_pair.first && i != distant_pair.second) {
            const default_type dist = (vertices[all_extrema[i]].dir - (vertices[all_extrema[distant_pair.first]].dir)).cross (vertices[all_extrema[i]].dir - vertices[all_extrema[distant_pair.second]].dir).norm() / (vertices[all_extrema[distant_pair.second]].dir - vertices[all_extrema[distant_pair.first]].dir).norm();
            if (dist > max_dist) {
              max_dist = dist;
              third_point = i;
            }
          }
        }
        assert (third_point != 6);

        // Does this have to be done in order?
        // It appears not - however random deletion of entries _is_ required
        //std::multiset<Plane, PlaneComp> planes;
        std::list<Plane> planes;
        planes.push_back (Plane (vertices, all_extrema[distant_pair.first], all_extrema[distant_pair.second], all_extrema[third_point]));
        // Find the most distant point to this plane, and use it as the tip point of the tetrahedon
        const Plane base_plane = *planes.begin();
        index_type fourth_point = vertices.size();
        max_dist = 0.0;
        for (index_type i = 0; i != vertices.size(); ++i) {
          // Use the reverse of the base plane normal - searching the other hemisphere
          const default_type dist = vertices[i].dir.dot (-base_plane.normal);
          if (dist > max_dist) {
            max_dist = dist;
            fourth_point = i;
          }
        }
        assert (fourth_point != vertices.size());
        planes.push_back (Plane (vertices, base_plane.indices[0], fourth_point, base_plane.indices[1]));
        planes.push_back (Plane (vertices, base_plane.indices[1], fourth_point, base_plane.indices[2]));
        planes.push_back (Plane (vertices, base_plane.indices[2], fourth_point, base_plane.indices[0]));

        vector<Plane> hull;

        // Speedup: Only test those directions that have not yet been incorporated into any plane
        BitSet assigned (vertices.size());
        assigned[base_plane.indices[0]] = true;
        assigned[base_plane.indices[1]] = true;
        assigned[base_plane.indices[2]] = true;
        assigned[fourth_point] = true;
        size_t assigned_counter = 4;

        while (planes.size()) {
          Plane current (planes.back());
          index_type max_index = vertices.size();
          default_type max_dist = current.dist;
          for (size_t d = 0; d != vertices.size(); ++d) {
            if (!assigned[d]) {
              const default_type dist = vertices[d].dir.dot (current.normal);
              if (dist > max_dist) {
                max_dist = dist;
                max_index = d;
              }
            }
          }

          if (max_index == vertices.size()) {
            hull.push_back (current);
            planes.pop_back();
          } else {

            // Identify all planes that this extremum point is above
            // More generally this would need to be constrained to only those faces adjacent to the
            //   current plane, but because the data are on the sphere a complete search should be fine

            // TODO Using an alternative data structure, where both faces connected to each
            //   edge are stored and tracked, would speed this up considerably
            vector< std::list<Plane>::iterator > all_planes;
            for (std::list<Plane>::iterator p = planes.begin(); p != planes.end(); ++p) {
              if (!p->includes (max_index) && vertices[max_index].dir.dot (p->normal) > p->dist)
                all_planes.push_back (p);
            }

            // Find the matching edges from multiple faces, and construct new triangles going up to the new point
            // Remove any shared edges; non-shared edges are the projection horizon
            std::set<std::pair<index_type, index_type>> horizon;
            for (auto& p : all_planes) {
              for (size_t edge_index = 0; edge_index != 3; ++edge_index) {
                std::pair<index_type, index_type> edge;
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
              planes.push_back (Plane (vertices, h.first, h.second, max_index));

            // Delete the used faces
            for (auto i : all_planes)
              planes.erase (i);

            // This point no longer needs to be tested
            assigned[max_index] = true;
            ++assigned_counter;

          }
        }

        for (auto& current : hull) {
          // Each of these three directions is adjacent
          // However: Each edge may have already been added from other triangles
          for (size_t edge = 0; edge != 6; ++edge) {
            index_type from = 0, to = 0;
            switch (edge) {
              case 0: from = vertices[current.indices[0]].index; to = vertices[current.indices[1]].index; break;
              case 1: from = vertices[current.indices[1]].index; to = vertices[current.indices[0]].index; break;
              case 2: from = vertices[current.indices[1]].index; to = vertices[current.indices[2]].index; break;
              case 3: from = vertices[current.indices[2]].index; to = vertices[current.indices[1]].index; break;
              case 4: from = vertices[current.indices[0]].index; to = vertices[current.indices[2]].index; break;
              case 5: from = vertices[current.indices[2]].index; to = vertices[current.indices[0]].index; break;
            }
            if (std::find ((*this)[from].begin(), (*this)[from].end(), to) == (*this)[from].end())
              (*this)[from].push_back (to);
          }
        }

        for (auto& i : *this)
          std::sort (i.begin(), i.end());
      }



      }
    }
  }
}

