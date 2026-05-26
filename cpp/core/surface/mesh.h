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

#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>

#include "header.h"
#include "image.h"
#include "transform.h"

#include "algo/copy.h"
#include "algo/loop.h"

#include "surface/types.h"

namespace MR::Surface::Filter {
class Smooth;
} // namespace MR::Surface::Filter

namespace MR::Surface {

class Mesh {

public:
  Mesh(const std::filesystem::path &);

  Mesh(const Mesh &that) = default;

  Mesh(Mesh &&that) noexcept
      : vertices(std::move(that.vertices)),
        normals(std::move(that.normals)),
        triangles(std::move(that.triangles)),
        quads(std::move(that.quads)) {}

  Mesh() {}

  Mesh &operator=(Mesh &&that) noexcept {
    vertices = std::move(that.vertices);
    normals = std::move(that.normals);
    triangles = std::move(that.triangles);
    quads = std::move(that.quads);
    return *this;
  }

  Mesh &operator=(const Mesh &that) {
    vertices = that.vertices;
    normals = that.normals;
    triangles = that.triangles;
    quads = that.quads;
    return *this;
  }

  void load(VertexList &&v, TriangleList &&p) {
    vertices = std::move(v);
    normals.clear();
    triangles = std::move(p);
    quads.clear();
  }
  void load(const VertexList &v, const TriangleList &p) {
    vertices = v;
    normals.clear();
    triangles = p;
    quads.clear();
  }

  void load(VertexList &&v, QuadList &&p) {
    vertices = std::move(v);
    normals.clear();
    triangles.clear();
    quads = std::move(p);
  }
  void load(const VertexList &v, const QuadList &p) {
    vertices = v;
    normals.clear();
    triangles.clear();
    quads = p;
  }

  void load(VertexList &&v, TriangleList &&p, QuadList &&q) {
    vertices = std::move(v);
    normals.clear();
    triangles = std::move(p);
    quads = std::move(q);
  }
  void load(const VertexList &v, const TriangleList &p, const QuadList &q) {
    vertices = v;
    normals.clear();
    triangles = p;
    quads = q;
  }

  void load(VertexList &&v, VertexList &&n, TriangleList &&p, QuadList &&q) {
    vertices = std::move(v);
    normals = std::move(n);
    triangles = std::move(p);
    quads = std::move(q);
  }
  void load(const VertexList &v, const VertexList &n, const TriangleList &p, const QuadList &q) {
    vertices = v;
    normals = n;
    triangles = p;
    quads = q;
  }

  void clear() {
    vertices.clear();
    normals.clear();
    triangles.clear();
    quads.clear();
  }

  void save(const std::filesystem::path &, const bool binary = false) const;

  [[nodiscard]] vertex_index_type num_vertices() const { return vertices.size(); }
  [[nodiscard]] size_t num_triangles() const { return triangles.size(); }
  [[nodiscard]] size_t num_quads() const { return quads.size(); }
  [[nodiscard]] size_t num_polygons() const { return triangles.size() + quads.size(); }

  [[nodiscard]] bool have_normals() const { return !normals.empty(); }
  void calculate_normals();

  [[nodiscard]] std::string get_name() const { return name; }
  void set_name(std::string_view n) { name = n; }

  [[nodiscard]] const Vertex &vert(const vertex_index_type i) const {
    assert(i < vertices.size());
    return vertices[i];
  }
  [[nodiscard]] const Vertex &norm(const vertex_index_type i) const {
    assert(i < normals.size());
    return normals[i];
  }
  [[nodiscard]] const Triangle &tri(const size_t i) const {
    assert(i < triangles.size());
    return triangles[i];
  }
  [[nodiscard]] const Quad &quad(const size_t i) const {
    assert(i < quads.size());
    return quads[i];
  }

  [[nodiscard]] const VertexList &get_vertices() const { return vertices; }
  [[nodiscard]] const VertexList &get_normals() const { return normals; }
  [[nodiscard]] const TriangleList &get_triangles() const { return triangles; }
  [[nodiscard]] const QuadList &get_quads() const { return quads; }

  void load_triangle_vertices(VertexList &, const size_t) const;
  void load_quad_vertices(VertexList &, const size_t) const;

protected:
  VertexList vertices;
  VertexList normals;
  TriangleList triangles;
  QuadList quads;

private:
  std::string name;

  void load_vtk(const std::filesystem::path &);
  void load_stl(const std::filesystem::path &);
  void load_obj(const std::filesystem::path &);
  void load_fs(const std::filesystem::path &);
  void save_vtk(const std::filesystem::path &, const bool) const;
  void save_stl(const std::filesystem::path &, const bool) const;
  void save_obj(const std::filesystem::path &) const;

  void verify_data() const;

  friend class MeshMulti;
  friend class Filter::Smooth;
  template <class ImageType> void mesh2image(const ImageType &, Mesh &, const default_type);
};

} // namespace MR::Surface
