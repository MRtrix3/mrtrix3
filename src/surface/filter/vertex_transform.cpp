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

#include "surface/filter/vertex_transform.h"
#include "file/nifti_utils.h"
#include "interp/linear.h"
#include "exception.h"
#include "math/SH.h"
#include "image.h"
#include "image_helpers.h"

class Warper {
public:
  Warper(const MR::Image<float>& warp_image)
      : interp(warp_image) {}

  Eigen::Vector3f pos(const Eigen::Vector3f& x) {
    Eigen::Vector3f p;
    if (interp.scanner(x.cast<double>())) {
      interp.index(3) = 0;
      p[0] = interp.value();
      interp.index(3) = 1;
      p[1] = interp.value();
      interp.index(3) = 2;
      p[2] = interp.value();
    }
    return p;
  }

private:
  MR::Interp::Linear<MR::Image<float>> interp;
};

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
        
        // Initialization of variables outside the switch statement
        vector<size_t> axes(3);
        Eigen::Vector3d cras(3, 1);
        Eigen::Transform<double, 3, 18>* M_ptr = nullptr;
        std::unique_ptr<Interp::Linear<Image<float>>> interp_ptr;


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
            {
              Eigen::Transform<double, 3, 18> M = File::NIfTI::adjust_transform(header, axes);
              M_ptr = &M;
              for (size_t i = 0; i < 3; i++) {
                cras[i] = (*M_ptr)(i, 3);
                for (size_t j = 0; j < 3; j++) {
                  cras[i] += 0.5 * header.size(axes[j]) * header.spacing(axes[j]) * (*M_ptr)(i, j);
                }
              }
              for (size_t i = 0; i != V; ++i) {
                vertices.push_back(in.vert(i) + cras);
              }
            }
            break;

          case transform_t::WARP:
            {
              Warper warper(warp_image);

              for (size_t i = 0; i != V; ++i) {
                Vertex v = in.vert(i);
                auto warped_vertex = warper.pos(v.cast<float>());
                if (warped_vertex.allFinite()) {
                  vertices.push_back(warped_vertex.cast<double>());
                }
              }
            }
            break;

          default:
            break;
        }
        
        out.load (vertices, normals, in.get_triangles(), in.get_quads());
      }


    }
  }
}
