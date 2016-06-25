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

#ifndef __surface_mesh_h__
#define __surface_mesh_h__


#include <fstream>
#include <vector>
#include <stdint.h>

#include "header.h"
#include "image.h"
#include "transform.h"

#include "algo/copy.h"
#include "algo/loop.h"

#include "surface/types.h"



namespace MR
{
  namespace Surface
  {



    class Mesh {

      public:
        Mesh (const std::string&);

        Mesh (const Mesh& that) = default;

        Mesh (Mesh&& that) :
            vertices (std::move (that.vertices)),
            normals (std::move (that.normals)),
            triangles (std::move (that.triangles)),
            quads (std::move (that.quads)) { }

        Mesh() { }

        Mesh& operator= (Mesh&& that) {
          vertices = std::move (that.vertices);
          normals = std::move (that.normals);
          triangles = std::move (that.triangles);
          quads = std::move (that.quads);
          return *this;
        }


        void load (VertexList&& v, TriangleList&& p) {
          vertices = std::move (v);
          normals.clear();
          triangles = std::move (p);
          quads.clear();
        }
        void load (const VertexList& v, const TriangleList& p) {
          vertices = v;
          normals.clear();
          triangles = p;
          quads.clear();
        }

        void load (VertexList&& v, QuadList&& p) {
          vertices = std::move (v);
          normals.clear();
          triangles.clear();
          quads = std::move (p);
        }
        void load (const VertexList& v, const QuadList& p) {
          vertices = v;
          normals.clear();
          triangles.clear();
          quads = p;
        }

        void load (VertexList&& v, TriangleList&& p, QuadList&& q) {
          vertices = std::move (v);
          normals.clear();
          triangles = std::move (p);
          quads = std::move (q);
        }
        void load (const VertexList& v, const TriangleList& p, const QuadList& q) {
          vertices = v;
          normals.clear();
          triangles = p;
          quads = q;
        }


        void transform_first_to_realspace (const Header&);
        void transform_realspace_to_first (const Header&);
        void transform_voxel_to_realspace (const Header&);
        void transform_realspace_to_voxel (const Header&);

        void save (const std::string&, const bool binary = false) const;

        void output_pve_image (const Header&, const std::string&);

        void smooth (const float, const float);

        size_t num_vertices() const { return vertices.size(); }
        size_t num_triangles() const { return triangles.size(); }
        size_t num_quads() const { return quads.size(); }
        size_t num_polygons() const { return triangles.size() + quads.size(); }

        bool have_normals() const { return normals.size(); }
        void calculate_normals();

        const std::string& get_name() const { return name; }
        void set_name (const std::string& s) { name = s; }

        const Vertex&   vert (const size_t i) const { assert (i < vertices .size()); return vertices[i]; }
        const Vertex&   norm (const size_t i) const { assert (i < normals  .size()); return normals[i]; }
        const Triangle& tri  (const size_t i) const { assert (i < triangles.size()); return triangles[i]; }
        const Quad&     quad (const size_t i) const { assert (i < quads    .size()); return quads[i]; }


      protected:
        VertexList vertices;
        VertexList normals;
        TriangleList triangles;
        QuadList quads;


      private:
        std::string name;

        void load_vtk (const std::string&);
        void load_stl (const std::string&);
        void load_obj (const std::string&);
        void save_vtk (const std::string&, const bool) const;
        void save_stl (const std::string&, const bool) const;
        void save_obj (const std::string&) const;

        void verify_data() const;

        void load_triangle_vertices (VertexList&, const size_t) const;
        void load_quad_vertices     (VertexList&, const size_t) const;

        Eigen::Vector3 calc_normal (const Triangle&) const;
        Eigen::Vector3 calc_normal (const Quad&) const;

        float calc_area (const Triangle&) const;
        float calc_area (const Quad&) const;

        friend class MeshMulti;

    };



  }
}

#endif

