/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#include "fixel/correspondence/mapping.h"

#include "header.h"
#include "image.h"


namespace MR {
  namespace Fixel {
    namespace Correspondence {





      Mapping::Mapping (const uint32_t source_fixels, const uint32_t target_fixels) :
          source_fixels (source_fixels),
          target_fixels (target_fixels),
          M (target_fixels, vector<uint32_t>()) { }


      Mapping::Mapping (const std::string& directory)
      {
        load (directory);
      }



      void Mapping::load (const std::string& directory, const bool import_inverse)
      {
        const std::string dir_string = import_inverse ? "inverse" : "forward";
        Image<uint32_t> index_image = Image<uint32_t>::open (Path::join (directory, "index_" + dir_string + ".mif"));
        Image<uint32_t> fixels_image = Image<uint32_t>::open (Path::join (directory, "fixels_" + dir_string + ".mif"));
        Header converse_index_header = Header::open (Path::join (directory, std::string("index_") + (import_inverse ? "forward" : "inverse") + ".mif"));

        M.assign (index_image.size(0), vector<uint32_t>());
        for (uint32_t t_index = 0; t_index != index_image.size(0); ++t_index) {
          index_image.index(0) = t_index;
          index_image.index(1) = 0;
          const uint32_t count = index_image.value();
          index_image.index(1) = 1;
          const uint32_t offset = index_image.value();
          M[t_index].reserve (count);
          fixels_image.index(0) = offset;
          for (uint32_t i = 0; i != count; ++i) {
            M[t_index].push_back (fixels_image.value());
            fixels_image.index(0)++;
          }
        }
        source_fixels = converse_index_header.size(0);
        target_fixels = index_image.size(0);
      }



      void Mapping::save (const std::string& directory) const
      {
        File::mkdir (directory);
        save (directory, false);
        save (directory, true);
      }


      vector< vector<uint32_t> > Mapping::inverse() const
      {
        vector< vector<uint32_t> > Minv (source_fixels, vector<uint32_t>());
        for (uint32_t t_index = 0; t_index != target_fixels; ++t_index) {
          for (auto s_index : M[t_index])
            Minv[s_index].push_back (t_index);
        }
        return Minv;
      }



      void Mapping::save (const std::string& directory, const bool export_inverse) const
      {
        vector< vector<uint32_t> > Minv;
        if (export_inverse)
          Minv = inverse();
        const vector< vector<uint32_t> >& data (export_inverse ? Minv : M);
        const std::string dir_string = (export_inverse ? "inverse" : "forward");
        const std::string index_path = Path::join (directory, "index_" + dir_string + ".mif");
        const std::string fixels_path = Path::join (directory, "fixels_" + dir_string + ".mif");

        uint32_t fixel_map_count = 0;
        for (const auto& row : data)
          fixel_map_count += row.size();

        Header H_index;
        H_index.ndim() = 3;
        H_index.size(0) = data.size();
        H_index.size(1) = 2;
        H_index.size(2) = 1;
        H_index.stride(0) = 2;
        H_index.stride(1) = 1;
        H_index.stride(2) = 3;
        H_index.spacing(0) = H_index.spacing(1) = H_index.spacing(2) = 1.0;
        H_index.transform() = transform_type::Identity();
        H_index.datatype() = DataType::UInt32;
        H_index.datatype().set_byte_order_native();
        H_index.keyval()["fixels"] = "fixels_" + dir_string + ".mif";

        Header H_fixels;
        H_fixels.ndim() = 3;
        H_fixels.size(0) = fixel_map_count;
        H_fixels.size(1) = 1;
        H_fixels.size(2) = 1;
        H_fixels.stride(0) = 1;
        H_fixels.stride(1) = 2;
        H_fixels.stride(2) = 3;
        H_fixels.spacing(0) = H_fixels.spacing(1) = H_fixels.spacing(2) = 1.0;
        H_fixels.transform() = transform_type::Identity();
        H_fixels.datatype() = DataType::UInt32;
        H_fixels.datatype().set_byte_order_native();
        H_fixels.keyval()["index"] = "index_" + dir_string + ".mif";

        Image<uint32_t> index_image = Image<uint32_t>::create (index_path, H_index);
        Image<uint32_t> fixels_image = Image<uint32_t>::create (fixels_path, H_fixels);

        for (size_t t_index = 0; t_index != data.size(); ++t_index) {
          index_image.index(0) = t_index;
          index_image.index(1) = 0;
          index_image.value() = data[t_index].size();
          index_image.index(1) = 1;
          index_image.value() = fixels_image.index(0);
          for (auto s_index : data[t_index]) {
            fixels_image.value() = s_index;
            fixels_image.index(0) += 1;
          }
        }

      }


    }
  }
}
