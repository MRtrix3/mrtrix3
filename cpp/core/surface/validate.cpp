/* Copyright (c) 2008-2026 the MRtrix3 contributors.
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

#include "surface/validate.h"

#include <algorithm>
#include <map>
#include <numeric>
#include <set>
#include <tuple>
#include <vector>

#include "app.h"
#include "exception.h"
#include "mrtrix.h"
#include "surface/mesh.h"
#include "surface/mesh_multi.h"
#include "surface/types.h"

namespace MR::Surface {

namespace {

// An undirected edge, stored with the smaller vertex index first.
using Edge = std::pair<vertex_index_type, vertex_index_type>;

// Per-polygon record in the edge map.
struct EdgeEntry {
  size_t poly_index;
  bool forward; // true  = polygon traverses this edge as (lo → hi)
                // false = polygon traverses this edge as (hi → lo)
};

// Path-halving union-find.
size_t uf_find(std::vector<size_t> &parent, size_t x) {
  while (parent[x] != x) {
    parent[x] = parent[parent[x]];
    x = parent[x];
  }
  return x;
}

void uf_union(std::vector<size_t> &parent, size_t a, size_t b) {
  a = uf_find(parent, a);
  b = uf_find(parent, b);
  if (a != b)
    parent[a] = b;
}

// Add all directed edges of a polygon to the edge map.
// poly_idx is the global polygon index (triangles first, then quads).
template <vertex_index_type N>
void add_polygon_edges(const Polygon<N> &poly,
                       const size_t poly_idx,
                       std::map<Edge, std::vector<EdgeEntry>> &edge_map) {
  for (size_t j = 0; j != N; ++j) {
    const size_t a = poly[j];
    const size_t b = poly[(j + 1) % N];
    const size_t lo = std::min(a, b);
    const size_t hi = std::max(a, b);
    edge_map[{lo, hi}].push_back({poly_idx, (a == lo)});
  }
}

} // namespace

void validate(const Mesh &mesh) {
  const VertexList &vertices = mesh.get_vertices();
  const TriangleList &triangles = mesh.get_triangles();
  const QuadList &quads = mesh.get_quads();

  const size_t nv = vertices.size();
  const size_t nt = triangles.size();
  const size_t nq = quads.size();
  const size_t np = nt + nq;

  if (np == 0)
    throw Exception("Mesh \"" + mesh.get_name() + "\" contains no polygons");

  // ---------------------------------------------------------------
  // Check 1: No disconnected vertices.
  // Every vertex must be referenced by at least one polygon.
  // ---------------------------------------------------------------
  {
    Eigen::Array<bool, Eigen::Dynamic, 1> referenced(Eigen::Array<bool, Eigen::Dynamic, 1>::Zero(nv));
    for (size_t i = 0; i != nt; ++i)
      for (size_t j = 0; j != 3; ++j)
        referenced[triangles[i][j]] = true;
    for (size_t i = 0; i != nq; ++i)
      for (size_t j = 0; j != 4; ++j)
        referenced[quads[i][j]] = true;
    const Eigen::Index unreferenced_count = nv - referenced.count();
    if (unreferenced_count > 0)
      throw Exception("Mesh \"" + mesh.get_name() + "\": " +                                                //
                      str(unreferenced_count) + (unreferenced_count > 1 ? " vertices are" : " vertex is") + //
                      " not referenced by any polygon (disconnected vertex)");                              //
  }

  // ---------------------------------------------------------------
  // Check 2: No duplicate vertices.
  // No two vertices may have identical (x, y, z) coordinates.
  // Note that this check is currently performed
  // based on floating-point equivalence;
  // very proximal but non-equal vertices will not be flagged.
  // ---------------------------------------------------------------
  {
    struct Compare {
      bool operator()(const Vertex &a, const Vertex &b) const {
        if (MR::abs(a[0] - b[0]) > Eigen::NumTraits<Vertex::Scalar>::dummy_precision())
          return a[0] < b[0];
        if (MR::abs(a[1] - b[1]) > Eigen::NumTraits<Vertex::Scalar>::dummy_precision())
          return a[1] < b[1];
        // Ensure that for two equal vertices,
        // both a < b and b < a return false
        if (MR::abs(a[2] - b[2]) < Eigen::NumTraits<Vertex::Scalar>::dummy_precision())
          return false;
        return a[2] < b[2];
      }
    };
    std::set<Vertex, Compare> pos_set;
    vertex_index_type duplicate_count = 0U;
    for (vertex_index_type i = 0; i != nv; ++i) {
      if (!pos_set.insert(vertices[i]).second)
        ++duplicate_count;
    }
    if (duplicate_count > 0)
      throw Exception("Mesh \"" + mesh.get_name() + "\": " +                 //
                      str(duplicate_count) + " duplicate " +                 //
                      (duplicate_count > 1 ? "vertices" : "vertex") +        //
                      " (precisely equivalent position to another vertex)"); //
  }

  // ---------------------------------------------------------------
  // Check 3: No duplicate polygons.
  // No two polygons may reference the exact same set of vertex indices.
  // Sorting the indices before comparison makes this independent of
  // winding order (winding is checked separately in check 6).
  // ---------------------------------------------------------------
  {
    std::set<std::vector<vertex_index_type>> seen;
    size_t duplicate_count = 0;
    auto check_poly = [&](const auto &poly, const size_t idx, const char *type) {
      std::vector<vertex_index_type> key(poly.size());
      for (size_t j = 0; j != poly.size(); ++j)
        key[j] = poly[j];
      std::sort(key.begin(), key.end());
      if (!seen.insert(key).second)
        ++duplicate_count;
    };
    for (size_t i = 0; i != nt; ++i)
      check_poly(triangles[i], i, "triangle");
    for (size_t i = 0; i != nq; ++i)
      check_poly(quads[i], i, "quad");
    if (duplicate_count > 0)
      throw Exception("Mesh \"" + mesh.get_name() + "\": " +                     //
                      str(duplicate_count) + " duplicate polygon" +              //
                      (duplicate_count > 1 ? "s" : "") + " detected" +           //
                      " (polygons referencing the exact same set of vertices)"); //
  }

  // ---------------------------------------------------------------
  // Build the edge map.
  // For every undirected edge {lo, hi} record which polygons contain
  // it and whether they traverse it in the forward (lo→hi) direction.
  // Global polygon index: triangles occupy [0, nt), quads [nt, np).
  // ---------------------------------------------------------------
  std::map<Edge, std::vector<EdgeEntry>> edge_map;
  for (size_t i = 0; i != nt; ++i)
    add_polygon_edges(triangles[i], i, edge_map);
  for (size_t i = 0; i != nq; ++i)
    add_polygon_edges(quads[i], nt + i, edge_map);

  // ---------------------------------------------------------------
  // Check 4: Closed surface.
  // Every edge must be shared by exactly two polygons.
  // Fewer than two → boundary edge (open surface).
  // More than two  → non-manifold edge.
  // ---------------------------------------------------------------
  {
    size_t boundary_count = 0;
    size_t nonmanifold_count = 0;
    for (const auto &[edge, entries] : edge_map) {
      switch (entries.size()) {
      case 1:
        ++boundary_count;
        break;
      case 2:
        break;
      default:
        ++nonmanifold_count;
      }
    }
    if (boundary_count > 0 || nonmanifold_count > 0)
      throw Exception("Mesh \"" + mesh.get_name() + "\": " +                                             //
                      (boundary_count > 0 ? (str(boundary_count) + " boundary edge" +                    //
                                             (boundary_count > 1 ? "s" : "") +                           //
                                             " (belong to only one polygon, hence non-closed surface)" + //
                                             (nonmanifold_count > 0 ? " and" : ""))                      //
                                          : "") +                                                        //
                      (nonmanifold_count > 0 ? (str(nonmanifold_count) + " non-manifold edge" +          //
                                                (nonmanifold_count > 1 ? "s" : "") +                     //
                                                " (belong to more than two polygons)")                   //
                                             : ""));                                                     //
  }
  // ---------------------------------------------------------------
  // Check 5: Single connected component.
  // Polygons connected by a shared edge are joined via union-find.
  // If any polygon ends up in a different component from polygon 0,
  // the surface has multiple disconnected pieces.
  // ---------------------------------------------------------------
  {
    std::vector<size_t> parent(np, size_t(0));
    for (const auto &[edge, entries] : edge_map) {
      // After check 4, every edge has exactly two entries.
      uf_union(parent, static_cast<size_t>(entries[0].poly_index), static_cast<size_t>(entries[1].poly_index));
    }
    std::set<size_t> unique_roots;
    for (size_t i = 0; i != np; ++i)
      unique_roots.insert(uf_find(parent, i));
    if (unique_roots.size() > 1)
      throw Exception("Mesh \"" + mesh.get_name() + "\": " +                                                  //
                      "surface is broken into " + str(unique_roots.size()) + " unique connected components"); //
  }

  // ---------------------------------------------------------------
  // Check 6: Consistent normal orientation.
  // For a coherently oriented surface, every undirected edge must be
  // traversed in opposite directions by its two adjacent polygons:
  // one forward (lo→hi) and one backward (hi→lo).
  // Two polygons traversing the same edge in the same direction means
  // their winding orders — and therefore their normals — are
  // inconsistent.
  // ---------------------------------------------------------------
  {
    size_t reversed_normal_count = 0;
    for (const auto &[edge, entries] : edge_map) {
      // After checks 4 and 5, every entry has exactly two elements.
      if (entries[0].forward == entries[1].forward)
        ++reversed_normal_count;
    }
    if (reversed_normal_count > 0)
      throw Exception("Mesh \"" + mesh.get_name() + "\": " +                                         //
                      str(reversed_normal_count) + " connected polygons with inconsistent normals"); //
  }
}

void debug_validate(const Mesh &mesh) {
  if (App::log_level < 3)
    return;
  try {
    validate(mesh);
    DEBUG("Mesh \"" + mesh.get_name() + "\" passed all validation checks");
  } catch (const Exception &e) {
    throw Exception(e, "Mesh \"" + mesh.get_name() + "\": validation failed");
  }
}

void debug_validate(const MeshMulti &meshes) {
  if (App::log_level < 3)
    return;
  Exception e_all;
  for (const auto &mesh : meshes) {
    try {
      validate(mesh);
    } catch (Exception &e) {
      for (size_t i = 0; i != e.num(); ++i)
        e_all.push_back(e[i]);
    }
  }
  if (e_all.num())
    throw e_all;
}

} // namespace MR::Surface
