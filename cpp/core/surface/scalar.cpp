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

#include "surface/scalar.h"

#include "file/matrix.h"

#include "surface/freesurfer.h"
#include <fmt/std.h>

#include <filesystem>

namespace MR::Surface {

Scalar::Scalar(const std::filesystem::path &path, const Mesh &mesh) {
  DEBUG("Attempting to load surface scalar file \"{}\"...", path);
  try {
    File::Matrix::load_vector(path);
  } catch (Exception &e) {
    DEBUG(e[0]);
    try {
      load_fs_w(path, mesh);
    } catch (Exception &e) {
      DEBUG(e[0]);
      try {
        load_fs_curv(path, mesh);
      } catch (Exception &e) {
        DEBUG(e[0]);
        throw Exception("Input surface scalar file \"{}\" not in supported format", path);
      }
    }
  }
  if (static_cast<size_t>(size()) != mesh.num_vertices())
    throw Exception("Input surface scalar file \"{}\" has incorrect number of vertices ({}, mesh has {})",
                    path,
                    size(),
                    mesh.num_vertices());
  name = path.filename().string();
}

void Scalar::save(const std::filesystem::path &path) const { File::Matrix::save_vector(*this, path); }

void Scalar::load_fs_w(const std::filesystem::path &path, const Mesh &mesh) {
  std::ifstream in(path, std::ios_base::in | std::ios_base::binary);
  if (!in)
    throw Exception("Error opening surface scalar file \"{}\"", path);

  FreeSurfer::get_BE<int16_t>(in); // 'latency'
  const int32_t num_entries = FreeSurfer::get_int24_BE(in);
  Base::operator=(Base::Zero(num_entries));
  for (int32_t i = 0; i != num_entries; ++i) {
    const int32_t index = FreeSurfer::get_int24_BE(in);
    const float value = FreeSurfer::get_BE<float>(in);
    if (static_cast<size_t>(index) >= mesh.num_vertices())
      throw Exception("Error opening file \"{}\" as FreeSurfer w-file:{}{},{}{})",
                      path, //
                      " invalid vertex index (",
                      index, //
                      " mesh has ",
                      mesh.num_vertices()); //
    if (!in.good())
      throw Exception("Error opening file \"{}\" as FreeSurfer w-file: truncated file", path);
    (*this)[index] = value;
  }
}

void Scalar::load_fs_curv(const std::filesystem::path &path, const Mesh &mesh) {
  std::ifstream in(path, std::ios_base::in | std::ios_base::binary);
  if (!in)
    throw Exception("Error opening surface scalar file \"{}\"", path);

  const int32_t magic_number = FreeSurfer::get_int24_BE(in);
  if (magic_number == FreeSurfer::new_curv_file_magic_number) {

    const int32_t num_vertices = FreeSurfer::get_BE<int32_t>(in);
    if (static_cast<size_t>(num_vertices) != mesh.num_vertices())
      throw Exception("Error opening file \"{}\" as Freesurfer curv file:{}{},{}{})",
                      path, //
                      " Incorrect number of vertices (",
                      num_vertices, //
                      " mesh has ",
                      mesh.num_vertices()); //

    const int32_t num_faces = FreeSurfer::get_BE<int32_t>(in);
    if (static_cast<size_t>(num_faces) != mesh.num_polygons())
      throw Exception("Error opening file \"{}\" as Freesurfer curv file:{}{},{}{})",
                      path, //
                      " Incorrect number of polygons (",
                      num_faces, //
                      " mesh has ",
                      mesh.num_polygons()); //

    const int32_t vals_per_vertex = FreeSurfer::get_BE<int32_t>(in);
    if (vals_per_vertex != 1)
      throw Exception("Error opening file \"{}\" as Freesurfer curv file:{}",
                      path,                                //
                      " Only support 1 value per vertex"); //

    (*this).resize(num_vertices);
    for (int32_t i = 0; i != num_vertices; ++i)
      (*this)[i] = FreeSurfer::get_BE<float>(in);

  } else {

    const int32_t num_vertices = magic_number;
    if (static_cast<size_t>(num_vertices) != mesh.num_vertices())
      throw Exception("Error opening file \"{}\" as Freesurfer curv file:{}{},{}{})",
                      path, //
                      " Incorrect number of vertices (",
                      num_vertices, //
                      " mesh has ",
                      mesh.num_vertices()); //

    const int32_t num_faces = FreeSurfer::get_int24_BE(in);
    if (static_cast<size_t>(num_faces) != mesh.num_polygons())
      throw Exception("Error opening file \"{}\" as Freesurfer curv file:{}{},{}{})",
                      path, //
                      " Incorrect number of polygons (",
                      num_faces, //
                      " mesh has ",
                      mesh.num_polygons()); //

    (*this).resize(mesh.num_vertices());
    for (int32_t i = 0; i != num_vertices; ++i)
      (*this)[i] = 0.01 * FreeSurfer::get_BE<int16_t>(in);
  }

  if (!in.good())
    throw Exception("Error opening file \"{}\" as Freesurfer curv file: Truncated file", path);
}

} // namespace MR::Surface
