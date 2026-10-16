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

#include "surface/filter/vertex_transform.h"

#include <array>

#include "axes.h"
#include "exception.h"
#include "transform.h"


namespace MR
{
  namespace Surface
  {
    namespace Filter
    {



      namespace
      {

        //! Scanner-space position of FreeSurfer's "c_ras" for an image
        /*! FreeSurfer surface vertices are stored in "surface RAS" (tkregister) space:
         *  the coordinate system of the conformed volume from which the surfaces were
         *  generated, translated such that its origin lies at voxel index
         *  (width/2, height/2, depth/2) of that volume. The scanner-space position of that
         *  index is FreeSurfer's c_ras, and is therefore the translation that maps surface
         *  vertices back into scanner space.
         *
         *  Note the use of size/2 rather than (size-1)/2: c_ras is displaced by half a voxel
         *  from the centre of the bounding box of voxel centres, in the direction in which
         *  FreeSurfer's own voxel indices increase along each axis. That displacement, and
         *  hence c_ras itself, consequently depends on the sense of FreeSurfer's voxel axes,
         *  and cannot be evaluated using the axis directions of an arbitrarily reoriented
         *  copy of the image. Those directions are however fixed by the FreeSurfer conformed
         *  volume convention, under which voxel indices increase toward Left, Inferior and
         *  Anterior respectively; they are therefore derived here from the image geometry
         *  alone, which is invariant to the axis order and data strides with which the image
         *  happens to be stored on the filesystem or loaded into memory.
         */
        Eigen::Vector3d freesurfer_cras (const Header& H)
        {
          const MR::Transform transform (H);
          Eigen::Vector3d halfsize;
          for (size_t axis = 0; axis != 3; ++axis)
            halfsize[axis] = 0.5 * (H.size (axis) - 1);
          const Axes::Shuffle shuffle = Axes::get_shuffle_to_make_RAS (H.transform());
          // Sense of each FreeSurfer voxel axis relative to the corresponding scanner axis
          const std::array<default_type, 3> freesurfer_polarity { { -1.0, 1.0, -1.0 } };
          Eigen::Vector3d halfstep (Eigen::Vector3d::Zero());
          for (size_t scanner_axis = 0; scanner_axis != 3; ++scanner_axis) {
            const size_t image_axis = shuffle.permutations[scanner_axis];
            const default_type polarity = freesurfer_polarity[scanner_axis] * (shuffle.flips[image_axis] ? -1.0 : 1.0);
            halfstep += 0.5 * polarity * transform.voxel2scanner.matrix().col (image_axis);
          }
          return (transform.voxel2scanner * halfsize) + halfstep;
        }

      }



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
            const Eigen::Vector3d cras = freesurfer_cras (header);
            for (size_t i = 0; i != V; ++i)
              vertices.push_back (in.vert(i) + cras);
            break;

        }

        out.load (vertices, normals, in.get_triangles(), in.get_quads());
      }



    }
  }
}


