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


#include "dwi/tractography/mapping/mapping.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Mapping {




        size_t determine_upsample_ratio (const Header& header, const float step_size, const float ratio)
        {
          size_t upsample_ratio = 1;
          if (step_size && std::isfinite (step_size))
            upsample_ratio = std::ceil (step_size / (std::min (header.spacing(0), std::min (header.spacing(1), header.spacing(2))) * ratio));
          return upsample_ratio;
        }

        size_t determine_upsample_ratio (const Header& header, const std::string& tck_path, const float ratio)
        {
          Properties properties;
          Reader<> reader (tck_path, properties);
          return determine_upsample_ratio (header, properties, ratio);
        }


        size_t determine_upsample_ratio (const Header& header, const Tractography::Properties& properties, const float ratio)
        {
          if (header.ndim() < 3)
            throw Exception ("Cannot perform streamline mapping on image with less than three dimensions");
          return determine_upsample_ratio (header, get_step_size (properties), ratio);
        }








        void generate_header (Header& header, const std::string& tck_file_path, const vector<default_type>& voxel_size)
        {

          Properties properties;
          Reader<> file (tck_file_path, properties);

          Streamline<> tck;
          size_t track_counter = 0;

          Eigen::Vector3f min_values ( Inf,  Inf,  Inf);
          Eigen::Vector3f max_values (-Inf, -Inf, -Inf);

          {
            ProgressBar progress ("creating new template image", 0);
            while (file (tck) && track_counter++ < MAX_TRACKS_READ_FOR_HEADER) {
              for (const auto& i : tck) {
                min_values[0] = std::min (min_values[0], i[0]);
                max_values[0] = std::max (max_values[0], i[0]);
                min_values[1] = std::min (min_values[1], i[1]);
                max_values[1] = std::max (max_values[1], i[1]);
                min_values[2] = std::min (min_values[2], i[2]);
                max_values[2] = std::max (max_values[2], i[2]);
              }
              ++progress;
            }
          }

          min_values -= Eigen::Vector3f (3.0*voxel_size[0], 3.0*voxel_size[1], 3.0*voxel_size[2]);
          max_values += Eigen::Vector3f (3.0*voxel_size[0], 3.0*voxel_size[1], 3.0*voxel_size[2]);

          header.name() = "tckmap image header";
          header.ndim() = 3;

          for (size_t i = 0; i != 3; ++i) {
            header.size(i) = std::ceil((max_values[i] - min_values[i]) / voxel_size[i]);
            header.spacing(i) = voxel_size[i];
            header.stride(i) = i+1;
          }

          header.transform().matrix().setIdentity();
          header.transform().translation() = min_values.cast<double>();
          file.close();
        }





        void oversample_header (Header& header, const vector<default_type>& voxel_size)
        {
          INFO ("oversampling header...");

          header.transform().translation() += header.transform().rotation() * Eigen::Vector3d (
              0.5 * (voxel_size[0] - header.spacing(0)),
              0.5 * (voxel_size[1] - header.spacing(1)),
              0.5 * (voxel_size[2] - header.spacing(2))
              );

          for (size_t n = 0; n < 3; ++n) {
            header.size(n) = std::ceil(header.size(n) * header.spacing(n) / voxel_size[n]);
            header.spacing(n) = voxel_size[n];
          }
        }





      }
    }
  }
}



