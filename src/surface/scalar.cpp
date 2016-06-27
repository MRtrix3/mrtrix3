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


#include "surface/scalar.h"

#include "math/math.h"

#include "surface/freesurfer.h"


namespace MR
{
  namespace Surface
  {



    Scalar::Scalar (const std::string& path, const Mesh& mesh)
    {
      try {
        values = load_vector (path);
      } catch (...) {
        try {
          load_fs_w (path, mesh);
        } catch (...) {
          try {
            load_fs_curv (path, mesh);
          } catch (...) {
            throw Exception ("Input surface scalar file \"" + path + "\" not in supported format");
          }
        }
      }
      if (size_t(values.size()) != mesh.num_vertices())
        throw Exception ("Input surface scalar file \"" + path + "\" has incorrect number of vertices");
      name = Path::basename (path);
    }



    void Scalar::save (const std::string& path) const
    {
      save_vector (values, path);
    }



    void Scalar::load_fs_w (const std::string& path, const Mesh& mesh)
    {
      std::ifstream in (path, std::ios_base::in | std::ios_base::binary);
      if (!in)
        throw Exception ("Error opening surface scalar file \"" + path + "\"");

      FreeSurfer::get_BE<int16_t> (in); // 'latency'
      const int32_t num_entries = FreeSurfer::get_int24_BE (in);
      values = vector_type::Zero (num_vertices);
      for (size_t i = 0; i != num_entries; ++i) {
        const int32_t index = FreeSurfer::get_int24_BE (in);
        const float value = FreeSurfer::get_BE<float> (in);
        if (index >= mesh.num_vertices())
          throw Exception ("Error opening file \"" + path + "\" as FreeSurfer w-file: invalid vertex index");
        if (!in.good())
          throw Exception ("Error opening file \"" + path + "\" as FreeSurfer w-file: truncated file");
        values[index] = value;
      }
    }



    void Scalar::load_fs_curv (const std::string& path, const Mesh& mesh)
    {
      std::ifstream in (path, std::ios_base::in | std::ios_base::binary);
      if (!in)
        throw Exception ("Error opening surface scalar file \"" + path + "\"");

      const int32_t magic_number = FreeSurfer::get_int24_BE (in);
      if (magic_number == FreeSurfer::new_curv_file_magic_number) {

        const int32_t num_vertices = FreeSurfer::get_BE<int32_t> (in);
        if (num_vertices != mesh.num_vertices())
          throw Exception ("Error opening file \"" + path + "\" as Freesurfer curv file: Incorrect number of vertices");

        const int32_t num_faces = FreeSurfer::get_BE<int32_t> (in);
        if (num_faces != mesh.num_polygons())
          throw Exception ("Error opening file \"" + path + "\" as Freesurfer curv file: Incorrect number of polygons");

        const int32_t vals_per_vertex = FreeSurfer::get_BE<int32_t> (in);
        if (vals_per_vertex != 1)
          throw Exception ("Error opening file \"" + path + "\" as Freesurfer curv file: Only support 1 value per vertex");

        values.resize (num_vertices);
        for (int32_t i = 0; i != num_vertices; ++i)
          values[i] = FreeSurfer::get_BE<float> (in);

      } else {

        const int32_t num_vertices = magic_number;
        if (num_vertices != mesh.num_vertices())
          throw Exception ("Error opening file \"" + path + "\" as Freesurfer curv file: Incorrect number of vertices");

        const int32_t num_faces = FreeSurfer::get_int24_BE (in);
        if (num_faces != mesh.num_polygons())
          throw Exception ("Error opening file \"" + path + "\" as Freesurfer curv file: Incorrect number of polygons");

        values.resize (mesh.num_vertices());
        for (int32_t i = 0; i != num_vertices; ++i)
          values[i] = 0.01 * FreeSurfer::get_BE<int16_t> (in);

      }

      if (!in.good())
        throw Exception ("Error opening file \"" + path + "\" as Freesurfer curv file: Truncated file");


    }




  }
}


