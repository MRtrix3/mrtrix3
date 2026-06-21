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

#include "axes.h"
#include "exception.h"
#include "file/nifti_utils.h"
#include "surface/types.h"
#include "types.h"

namespace MR::Surface::Filter {

void VertexTransform::operator()(const Mesh &in, Mesh &out) const {
  VertexList vertices;
  VertexList normals;
  const size_t V = in.num_vertices();
  vertices.reserve(V);
  if (in.have_normals())
    normals.reserve(V);
  switch (mode) {

  case mode_t::UNDEFINED:
    throw Exception("Error: VertexTransform must have the transform type set");

  case mode_t::FIRST2REAL:
    // VAR(in.vert(0).transpose());
    // VAR(fsl_image2scanner.matrix());
    // VAR((fsl_image2scanner * in.vert(0)).transpose());
    // throw Exception();
    // TODO Investigate use of get_flirt_transform() in transformconvert
    for (size_t i = 0; i != V; ++i)
      vertices.push_back(fsl_image2scanner * in.vert(i));
    if (in.have_normals()) {
      for (size_t i = 0; i != V; ++i)
        normals.push_back(fsl_image2scanner.rotation() * in.norm(i));
    }
    break;

  case mode_t::REAL2FIRST:
    for (size_t i = 0; i != V; ++i)
      vertices.push_back(fsl_scanner2image * in.vert(i));
    if (in.have_normals()) {
      for (size_t i = 0; i != V; ++i)
        normals.push_back(fsl_scanner2image.rotation() * in.norm(i));
    }
    break;

  case mode_t::VOXEL2REAL:
    for (size_t i = 0; i != V; ++i)
      vertices.push_back(disk_voxel2scanner * in.vert(i));
    if (in.have_normals()) {
      for (size_t i = 0; i != V; ++i)
        normals.push_back(disk_voxel2scanner.rotation() * in.norm(i));
    }
    break;

  case mode_t::REAL2VOXEL:
    for (size_t i = 0; i != V; ++i)
      vertices.push_back(disk_scanner2voxel * in.vert(i));
    if (in.have_normals()) {
      for (size_t i = 0; i != V; ++i)
        normals.push_back(disk_scanner2voxel.rotation() * in.norm(i));
    }
    break;

  case mode_t::FS2REAL:
    const Axes::permutations_type &axes = header.realignment().permutations();
    Eigen::Vector3d cras(3, 1);
    for (size_t i = 0; i < 3; i++) {
      cras[i] = disk_scanner2voxel.translation()[i];
      for (size_t j = 0; j < 3; j++)
        cras[i] += 0.5 * header.size(axes[j]) * header.spacing(axes[j]) * disk_scanner2voxel(i, j);
    }
    for (size_t i = 0; i != V; ++i)
      vertices.push_back(in.vert(i) + cras);
    break;
  }

  out.load(vertices, normals, in.get_triangles(), in.get_quads());
}

// TODO This is still not working...
// Is it possible that FIRST is itself doing its own internal realignment,
//   in which case we should assume that the vertices are defined with respect to a realigned image?
// In which case, we can temporarily do a test based on the MRtrix3 realigned transform;
//   but if that were to be the solution,
//   then would need to account for prospect of -config RealignTransform false

transform_type VertexTransform::make_fsl_image2scanner(const Header &H) const {
  transform_type unflip(transform_type::Identity());
  unflip.matrix()(0, 0) = -1.0;
  unflip.translation()[0] = (H.size(0) - 1) * H.spacing(0);
  return H.transform() * unflip;
}

// transform_type VertexTransform::make_fsl_image2scanner(const Header &H) const {
//   if (H.realignment().orig_transform().matrix().topLeftCorner<3, 3>().determinant() > 0.0)
//     return H.realignment().orig_transform();

//   // // Apply flip to axis 0
//   // // Borrowed from Header::realign_transform()
//   // transform_type T(H.realignment().orig_transform());
//   // const default_type length = static_cast<default_type>(H.size(0) - 1) * H.spacing(0);
//   // auto axis = T.matrix().col(0);
//   // auto translation = T.translation();
//   // for (size_t n = 0; n < 3; ++n) {
//   //   axis[n] = -axis[n];
//   //   translation[n] -= length * axis[n];
//   // }
//   // return T;
//   transform_type unflip(transform_type::Identity());
//   unflip.matrix()(0, 0) = -1.0;
//   // FIXME This needs to apply to the size of the first axis prior to permutation!
//   // FIXME This might still be choosing the wrong axis
//   //const size_t orig_first_axis = H.realignment().permutation(0);
//   const size_t orig_axis_zero = H.realignment().permutation(0) == 0 ? 0 :
//       (H.realignment().permutation(1) == 0 ? 1 : 2);
//   unflip.translation()[0] = ((H.size(orig_axis_zero) - 1) * H.spacing(orig_axis_zero));
//   return H.realignment().orig_transform() * unflip;
// }

} // namespace MR::Surface::Filter
