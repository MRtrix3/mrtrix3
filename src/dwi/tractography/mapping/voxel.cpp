/* Copyright (c) 2008-2017 the MRtrix3 contributors
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




