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
#include "surface/types.h"

namespace MR::Surface {

namespace {

// An undirected edge, stored with the smaller vertex index first.
using Edge = std::pair<uint32_t, uint32_t>;

// Per-polygon record in the edge map.
struct EdgeEntry {
  size_t poly_index;
  bool forward; // true  = polygon traverses this edge as (lo → hi)
                // false = polygon traverses this edge as (hi → lo)
};

// Path-halving union-find.
uint32_t uf_find(std::vector<uint32_t> &parent, uint32_t x) {
  while (parent[x] != x) {
    parent[x] = parent[parent[x]];
    x = parent[x];
  }
  return x;
}

void uf_union(std::vector<uint32_t> &parent, uint32_t a, uint32_t b) {
  a = uf_find(parent, a);
  b = uf_find(parent, b);
  if (a != b)
    parent[a] = b;
}

// Add all directed edges of a polygon to the edge map.
// poly_idx is the global polygon index (triangles first, then quads).
template <uint32_t N>
void add_polygon_edges(const Polygon<N> &poly,
                       const size_t poly_idx,
                       std::map<Edge, std::vector<EdgeEntry>> &edge_map) {
  for (size_t j = 0; j != N; ++j) {
    const uint32_t a = poly[j];
    const uint32_t b = poly[(j + 1) % N];
    const uint32_t lo = std::min(a, b);
    const uint32_t hi = std::max(a, b);
    edge_map[{lo, hi}].push_back({poly_idx, (a == lo)});
  }
}

} // namespace

void validate_mesh(const Mesh &mesh) {
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
    std::vector<bool> referenced(nv, false);
    for (size_t i = 0; i != nt; ++i)
      for (size_t j = 0; j != 3; ++j)
        referenced[triangles[i][j]] = true;
    for (size_t i = 0; i != nq; ++i)
      for (size_t j = 0; j != 4; ++j)
        referenced[quads[i][j]] = true;
    for (size_t i = 0; i != nv; ++i)
      if (!referenced[i])
        throw Exception("Mesh \"" + mesh.get_name() + "\": vertex " + str(i) +
                        " is not referenced by any polygon (disconnected vertex)");
  }

  // ---------------------------------------------------------------
  // Check 2: No duplicate vertices.
  // No two vertices may have identical (x, y, z) coordinates.
  // ---------------------------------------------------------------
  {
    // Map from exact vertex position to the first vertex index at that position.
    std::map<std::tuple<double, double, double>, size_t> pos_map;
    for (size_t i = 0; i != nv; ++i) {
      const Vertex &v = vertices[i];
      const auto key = std::make_tuple(v[0], v[1], v[2]);
      const auto [it, inserted] = pos_map.emplace(key, i);
      if (!inserted)
        throw Exception("Mesh \"" + mesh.get_name() + "\": vertex " + str(i) + " has the same position as vertex " +
                        str(it->second) + " (duplicate vertex)");
    }
  }

  // ---------------------------------------------------------------
  // Check 3: No duplicate polygons.
  // No two polygons may reference the same set of vertex indices.
  // Sorting the indices before comparison makes this independent of
  // winding order (winding is checked separately in check 6).
  // ---------------------------------------------------------------
  {
    std::set<std::vector<uint32_t>> seen;
    auto check_poly = [&](const auto &poly, const size_t idx, const char *type) {
      std::vector<uint32_t> key(poly.size());
      for (size_t j = 0; j != poly.size(); ++j)
        key[j] = poly[j];
      std::sort(key.begin(), key.end());
      if (!seen.insert(key).second)
        throw Exception("Mesh \"" + mesh.get_name() + "\": " + type + " " + str(idx) +
                        " is a duplicate of an earlier polygon");
    };
    for (size_t i = 0; i != nt; ++i)
      check_poly(triangles[i], i, "triangle");
    for (size_t i = 0; i != nq; ++i)
      check_poly(quads[i], i, "quad");
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
  for (const auto &[edge, entries] : edge_map) {
    if (entries.size() == 1)
      throw Exception("Mesh \"" + mesh.get_name() + "\": edge (" + str(edge.first) + ", " + str(edge.second) +
                      ") belongs to only one polygon"
                      " (boundary edge; surface is not closed)");
    if (entries.size() > 2)
      throw Exception("Mesh \"" + mesh.get_name() + "\": edge (" + str(edge.first) + ", " + str(edge.second) +
                      ") is shared by " + str(entries.size()) + " polygons (non-manifold edge)");
  }

  // ---------------------------------------------------------------
  // Check 5: Single connected component.
  // Polygons connected by a shared edge are joined via union-find.
  // If any polygon ends up in a different component from polygon 0,
  // the surface has multiple disconnected pieces.
  // ---------------------------------------------------------------
  {
    std::vector<uint32_t> parent(np);
    std::iota(parent.begin(), parent.end(), uint32_t(0));
    for (const auto &[edge, entries] : edge_map) {
      // After check 4, every edge has exactly two entries.
      uf_union(parent, static_cast<uint32_t>(entries[0].poly_index), static_cast<uint32_t>(entries[1].poly_index));
    }
    const uint32_t root = uf_find(parent, 0);
    for (size_t i = 1; i != np; ++i) {
      if (uf_find(parent, static_cast<uint32_t>(i)) != root)
        throw Exception("Mesh \"" + mesh.get_name() +
                        "\": surface is not a single connected component"
                        " (polygon " +
                        str(i) + " is in a separate piece)");
    }
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
  for (const auto &[edge, entries] : edge_map) {
    // After checks 4 and 5, every entry has exactly two elements.
    if (entries[0].forward == entries[1].forward)
      throw Exception("Mesh \"" + mesh.get_name() + "\": edge (" + str(edge.first) + ", " + str(edge.second) +
                      ") is traversed in the same direction by polygons " + str(entries[0].poly_index) + " and " +
                      str(entries[1].poly_index) + " (inconsistent normal orientation)");
  }
}

void debug_validate_mesh(const Mesh &mesh) {
  if (App::log_level < 3)
    return;
  try {
    validate_mesh(mesh);
    DEBUG("Mesh \"" + mesh.get_name() + "\" passed all validation checks");
  } catch (const Exception &e) {
    DEBUG("Mesh \"" + mesh.get_name() + "\": validation failed: " + e[0]);
  }
}

} // namespace MR::Surface
