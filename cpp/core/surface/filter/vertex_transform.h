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

#include "header.h"
#include "types.h"

#include "surface/filter/base.h"
#include "surface/mesh.h"
#include "surface/mesh_multi.h"

namespace MR::Surface::Filter {

class VertexTransform : public Base {
public:
  enum class mode_t { UNDEFINED, FIRST2REAL, REAL2FIRST, VOXEL2REAL, REAL2VOXEL, FS2REAL };

  VertexTransform(const Header &H)
      : header(H),
        disk_voxel2scanner(H.realignment().orig_transform() *
                           Eigen::DiagonalMatrix<default_type, 3>(H.spacing(0), H.spacing(1), H.spacing(2))),
        disk_scanner2voxel(disk_voxel2scanner.inverse()),
        fsl_image2scanner(make_fsl_image2scanner(H)),
        fsl_scanner2image(fsl_image2scanner.inverse()),
        mode(mode_t::UNDEFINED) {

    VAR(disk_scanner2voxel.matrix());
    VAR(disk_voxel2scanner.matrix());
    VAR(fsl_scanner2image.matrix());
    VAR(fsl_image2scanner.matrix());
  }

  void set_first2real() { mode = mode_t::FIRST2REAL; }
  void set_real2first() { mode = mode_t::REAL2FIRST; }
  void set_voxel2real() { mode = mode_t::VOXEL2REAL; }
  void set_real2voxel() { mode = mode_t::REAL2VOXEL; }
  void set_fs2real() { mode = mode_t::FS2REAL; }

  mode_t get_mode() const { return mode; }

  void operator()(const Mesh &, Mesh &) const override;

  void operator()(const MeshMulti &in, MeshMulti &out) const override { Base::operator()(in, out); }

private:
  const Header &header;
  const transform_type disk_voxel2scanner;
  const transform_type disk_scanner2voxel;
  const transform_type fsl_image2scanner;
  const transform_type fsl_scanner2image;
  mode_t mode;

  transform_type make_fsl_image2scanner(const Header &H) const;
};

} // namespace MR::Surface::Filter
