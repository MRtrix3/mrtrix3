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

#include "dwi/tractography/mapping/voxel.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Mapping {



        const Dixel::dir_index_type Dixel::invalid = std::numeric_limits<Dixel::dir_index_type>::max();



        std::ostream& operator<< (std::ostream& stream, const Voxel& v) {
          stream << "[" << Eigen::Vector3i(v).transpose() << "]: " << v.get_length();
          return stream;
        }

        std::ostream& operator<< (std::ostream& stream, const VoxelDEC& v) {
          stream << "[" << Eigen::Vector3i(v).transpose() << "]: [" << v.get_colour().transpose() << "] " << v.get_length();
          return stream;
        }

        std::ostream& operator<< (std::ostream& stream, const VoxelDir& v) {
          stream << "[" << Eigen::Vector3i(v).transpose() << "]: [" << v.get_dir().transpose() << "] " << v.get_length();
          return stream;
        }

        std::ostream& operator<< (std::ostream& stream, const Dixel& d) {
          stream << "[" << Eigen::Vector3i(d).transpose() << "] " << d.get_dir() << ": " << d.get_length();
          return stream;
        }

        std::ostream& operator<< (std::ostream& stream, const VoxelTOD& v) {
          stream << "[" << Eigen::Vector3i(v).transpose() << "]: " << v.get_tod().transpose() << " " << v.get_length();
          return stream;
        }



      }
    }
  }
}




