/*
   Copyright 2011 Brain Research Institute, Melbourne, Australia

   Written by Robert E. Smith, 2015.

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

#ifndef __mesh_vox2mesh_h__
#define __mesh_vox2mesh_h__


#include "mesh/mesh.h"



namespace MR
{
  namespace Mesh
  {


    // Function to convert a binary image into a mesh
    // Will want this to be templated so that it can be done from a buffer
    //   (e.g. in a dedicated command), or a scratch buffer (e.g. for the
    //   connectome tool)
    //
    // GL_QUADS is depreciated... Export triangles instead

    template <class VoxelType>
    void vox2mesh (const VoxelType& in, Mesh& out)
    {

        static const Point<int> steps[6] = { Point<int> ( 0,  0, -1),
                                             Point<int> ( 0, -1,  0),
                                             Point<int> (-1,  0,  0),
                                             Point<int> ( 0,  0,  1),
                                             Point<int> ( 0,  1,  0),
                                             Point<int> ( 1,  0,  0) };

        static const int plane_axes[6][2] = { {0, 1},
                                              {0, 2},
                                              {1, 2},
                                              {0, 1},
                                              {0, 2},
                                              {1, 2} };

        VoxelType vox (in), neighbour (in);
        VertexList vertices;
        //QuadList polygons;
        TriangleList polygons;
        std::map< Point<int>, size_t > pos2vertindex;

        // Perform all initial calculations in voxel space;
        //   only after the data has been written to a Mesh class will the conversion to
        //   real space take place
        //
        // Also, for initial calculations, do this such that a voxel location actually
        //   refers to the lower corner of the voxel; that way searches for existing
        //   vertices can be done using a simple map

        Image::LoopInOrder loop (vox, "converting mask image to mesh representation... ");
        for (loop.start (vox); loop.ok(); loop.next (vox)) {
          if (vox.value()) {

            for (size_t adj = 0; adj != 6; ++adj) {
              Image::Nav::set_pos (neighbour, vox);
              Image::Nav::step_pos (neighbour, steps[adj]);

              // May get an overflow here; can't guarantee order of an or operation
              //if (!Image::Nav::within_bounds (neighbour) || !neighbour.value()) {

              bool is_interface = !Image::Nav::within_bounds (neighbour);
              if (!is_interface)
                is_interface = !neighbour.value();
              if (is_interface) {

                // This interface requires four vertices; some may have already been
                //   created, others may not have.
                // Selection of these vertices from the central voxel depend on the
                //   direction to the empty neighbour voxel
                // Remember: integer locations map to the lower corner of the voxel

                Point<int> p (vox[0], vox[1], vox[2]);
                if (steps[adj].dot (Point<int> (1, 1, 1)) > 0)
                  p += steps[adj];

                // Break this voxel face into two triangles

                // Get the integer positions of the four vertices
                std::vector< Point<int> > voxels (4, p);
                voxels[1][plane_axes[adj][0]]++;
                voxels[2][plane_axes[adj][0]]++; voxels[2][plane_axes[adj][1]]++;
                voxels[3][plane_axes[adj][1]]++;

                for (size_t tri_index = 0; tri_index != 2; ++tri_index) {
                  Polygon<3> triangle_vertices;
                  // Triangle 0 uses vertices (0, 1, 2); triangle 1 uses vertices (0, 2, 3)
                  for (size_t out_vertex = 0; out_vertex != 3; ++out_vertex) {
                    const size_t in_vertex = out_vertex + (tri_index && out_vertex ? 1 : 0);
                    const auto existing = pos2vertindex.find (voxels[in_vertex]);
                    if (existing == pos2vertindex.end()) {
                      triangle_vertices[out_vertex] = vertices.size();
                      vertices.push_back (Vertex (float(voxels[in_vertex][0]) - 0.5f, float(voxels[in_vertex][1]) - 0.5f, float(voxels[in_vertex][2]) - 0.5f));
                    } else {
                      triangle_vertices[out_vertex] = existing->second;
                    }
                  }
                  polygons.push_back (triangle_vertices);
                }

              } // End checking for interface
            } // End looping over adjacent voxels

        } } // End looping over image

        // Write the result to the output class
        out.load (vertices, polygons);

    }


  }
}

#endif

