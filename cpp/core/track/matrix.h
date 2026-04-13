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


#ifndef __track_matrix_h__
#define __track_matrix_h__

#include "image.h"
#include "types.h"
#include "file/ofstream.h"

namespace MR
{
  namespace Track
  {
    namespace Matrix
    {

      using ind_type = int;

      class Reader
      {
        public:
            Reader(const std::string& folder);

            // Eigen::Matrix<float, Eigen::Dynamic, 1> operator[] (const size_t index) const;

            FORCE_INLINE size_t size() const { return index_image.size(0); }

            FORCE_INLINE const Image<ind_type>& get_index_image() const { return index_image; }
            FORCE_INLINE const Image<int>& get_streamlines_image() const { return streamlines_image; }
            FORCE_INLINE const Image<float>& get_values_image() const { return values_image; }

            Eigen::Matrix<float, Eigen::Dynamic, 1> get_scores(const size_t index) const;
            Eigen::Matrix<float, Eigen::Dynamic, 1> get_similar(const size_t index) const;

        private:
            std::string index_file;
            std::string streamlines_file;
            std::string values_file;
            Image<ind_type> index_image;
            Image<ind_type> streamlines_image;
            Image<float> values_image;
      };

      
    }
  }
}

#endif
