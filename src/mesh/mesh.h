/*
   Copyright 2011 Brain Research Institute, Melbourne, Australia

   Written by Robert E. Smith, 2011.

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

#ifndef __mesh_mesh_h__
#define __mesh_mesh_h__


#include <fstream>
#include <vector>
#include <stdint.h>

#include "point.h"

#include "image/buffer.h"
#include "image/buffer_scratch.h"
#include "image/copy.h"
#include "image/header.h"
#include "image/loop.h"
#include "image/nav.h"
#include "image/transform.h"
#include "image/voxel.h"


#include "math/matrix.h"



namespace MR
{
  namespace Mesh
  {


    typedef Point<float> Vertex;
    typedef std::vector<Vertex> VertexList;


    template <uint32_t vertices = 3>
    class Polygon
    {

      public:

        template <class T>
        Polygon (const T* const d)
        {
          for (size_t i = 0; i != vertices; ++i)
            indices[i] = d[i];
        }

        template <class C>
        Polygon (const C& d)
        {
          assert (d.size() == vertices);
          for (size_t i = 0; i != vertices; ++i)
            indices[i] = d[i];
        }

        Polygon()
        {
          memset (indices, 0, vertices * sizeof (uint32_t));
        }


        uint32_t  operator[] (const size_t i) const { assert (i < vertices); return indices[i]; }
        uint32_t& operator[] (const size_t i)       { assert (i < vertices); return indices[i]; }


      private:
        uint32_t indices[vertices];

    };

    typedef Polygon<3> Triangle;
    typedef std::vector<Triangle> TriangleList;
    typedef Polygon<4> Quad;
    typedef std::vector<Quad> QuadList;



    class Mesh {

      public:
        Mesh (const std::string&);
        Mesh() { }


        void load (VertexList&& v, TriangleList&& p) {
          vertices = v;
          triangles = p;
          quads.clear();
        }
        void load (const VertexList& v, const TriangleList& p) {
          vertices = v;
          triangles = p;
          quads.clear();
        }

        void load (VertexList&& v, QuadList&& p) {
          vertices = v;
          triangles.clear();
          quads = p;
        }
        void load (const VertexList& v, const QuadList& p) {
          vertices = v;
          triangles.clear();
          quads = p;
        }


        void transform_first_to_realspace (const Image::Header&);
        void transform_voxel_to_realspace (const Image::Header&);
        void transform_realspace_to_voxel (const Image::Header&);

        void save (const std::string&, const bool binary = true) const;

        void output_pve_image (const Image::Header&, const std::string&);

        size_t num_vertices() const { return vertices.size(); }
        size_t num_triangles() const { return triangles.size(); }
        size_t num_quads() const { return quads.size(); }
        size_t num_polygons() const { return triangles.size() + quads.size(); }

        const Vertex&   vert (const size_t i) const { return vertices[i]; }
        const Triangle& tri  (const size_t i) const { return triangles[i]; }
        const Quad&     quad (const size_t i) const { return quads[i]; }


      protected:
        VertexList vertices;
        TriangleList triangles;
        QuadList quads;


      private:
        void load_vtk (const std::string&);
        void load_stl (const std::string&);
        void save_vtk (const std::string&, const bool) const;
        void save_stl (const std::string&, const bool) const;

        void verify_data() const;

        void load_triangle_vertices (VertexList&, const size_t) const;
        void load_quad_vertices     (VertexList&, const size_t) const;

        Point<float> calc_normal (const Triangle&) const;
        Point<float> calc_normal (const Quad&) const;

    };





  }
}

#endif

