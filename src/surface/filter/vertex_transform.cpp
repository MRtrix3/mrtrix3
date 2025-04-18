/* Copyright (c) 2008-2025 the MRtrix3 contributors.
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

#include "surface/filter/vertex_transform.h"

#include "axes.h"
#include "exception.h"
#include "file/nifti_utils.h"


namespace MR
{
  namespace Surface
  {
    namespace Filter
    {



      void VertexTransform::operator() (const Mesh& in, Mesh& out) const
      {
        VertexList vertices, normals;
        const size_t V = in.num_vertices();
        vertices.reserve (V);
        if (in.have_normals())
          normals.reserve (V);
        switch (mode) {

          case transform_t::UNDEFINED:
            throw Exception ("Error: VertexTransform must have the transform type set");

          case transform_t::FIRST2REAL:
            for (size_t i = 0; i != V; ++i) {
              Vertex v = in.vert(i);
              v[0] = ((header.size(0)-1) * header.spacing(0)) - v[0];
              vertices.push_back (transform.image2scanner * v);
            }
            if (in.have_normals()) {
              for (size_t i = 0; i != V; ++i) {
                Vertex n = in.norm(i);
                n[0] = -n[0];
                normals.push_back (transform.image2scanner.rotation() * n);
              }
            }
            break;

          case transform_t::REAL2FIRST:
            for (size_t i = 0; i != V; ++i) {
              Vertex v = in.vert(i);
              v = transform.scanner2image * v;
              v[0] = ((header.size(0)-1) * header.spacing(0)) - v[0];
              vertices.push_back (std::move (v));
            }
            if (in.have_normals()) {
              for (size_t i = 0; i != V; ++i) {
                Vertex n = transform.scanner2image.rotation() * in.norm(i);
                n[0] = -n[0];
                normals.push_back (std::move (n));
              }
            }
            break;

          case transform_t::VOXEL2REAL:
            for (size_t i = 0; i != V; ++i)
              vertices.push_back (transform.voxel2scanner * in.vert(i));
            if (in.have_normals()) {
              for (size_t i = 0; i != V; ++i)
                normals.push_back (transform.voxel2scanner.rotation() * in.norm(i));
            }
            break;

          case transform_t::REAL2VOXEL:
            for (size_t i = 0; i != V; ++i)
              vertices.push_back (transform.scanner2voxel * in.vert(i));
            if (in.have_normals()) {
              for (size_t i = 0; i != V; ++i)
                normals.push_back (transform.scanner2voxel.rotation() * in.norm(i));
            }
            break;

          case transform_t::FS2REAL:
            auto M = header.realignment().orig_transform();
            const Axes::permutations_type& axes = header.realignment().permutations();
            Eigen::Vector3d cras( 3, 1 );
            for ( size_t i = 0; i < 3; i++ )
            {
              cras[ i ] = M( i, 3 );
              for ( size_t j = 0; j < 3; j++ )
                cras[ i ] += 0.5 * header.size( axes[ j ] )
                                 * header.spacing( axes[ j ] ) * M( i, j );
            }
            for ( size_t i = 0; i != V; ++i )
            {
              vertices.push_back ( in.vert(i) + cras );
            }
            break;

        }

        out.load (vertices, normals, in.get_triangles(), in.get_quads());
      }



    }
  }
}


