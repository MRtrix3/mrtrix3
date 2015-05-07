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

        // make polygon accessible
        uint32_t indices[ vertices ];

      private:
        //uint32_t indices[vertices];

    };


    typedef std::vector< Polygon<3> > PolygonList;


    class Mesh {

      public:
        Mesh (const std::string& path)
        {
          if (path.substr (path.size() - 4) == ".vtk")
            load_vtk (path);
          else
            throw Exception ("Input mesh file not in supported format");
        }


        void transform_first_to_realspace (const Image::Header&);
        void transform_realspace_to_voxel (const Image::Header&);

        void output_pve_image (const Image::Header&, const std::string&);

        // make vertices & polygons accessible
        VertexList vertices;
        PolygonList polygons;

      private:
        //VertexList vertices;
        //PolygonList polygons;


        void load_vtk (const std::string&);

        void verify_data() const;

        void load_polygon_vertices (VertexList&, const size_t) const;

    };




  }
}

#endif

