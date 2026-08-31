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


#include "track/matrix.h"

#include "app.h"
#include "image.h"
#include "thread_queue.h"
#include "types.h"
#include "file/ofstream.h"
#include "file/path.h"
#include "file/utils.h"
#include "fixel/helpers.h"

#include "dwi/tractography/mapping/loader.h"
#include "dwi/tractography/mapping/mapper.h"
#include "dwi/tractography/mapping/voxel.h"
#include "dwi/tractography/streamline.h"

namespace MR::Track::Matrix
{

  Reader::Reader (std::string_view folder) {
    index_file = std::string(folder) + "/index.mif";
    streamlines_file = std::string(folder) + "/streamlines.mif";
    values_file = std::string(folder) + "/values.mif";

    index_image = Image<ind_type>::open(index_file);
    streamlines_image = Image<ind_type>::open(streamlines_file);
    values_image = Image<float>::open(values_file);
  }


  Eigen::Matrix<float, Eigen::Dynamic, 1> Reader::get_scores(const size_t index) const {
    Eigen::Matrix<float, Eigen::Dynamic, 1> result;
    
    // for thread safety
    Image<int> index_img (index_image);
    Image<float> score_img (values_image);

    // get howmany and offset
    index_img.move_index(0, index);
    size_t howmany = index_img.get_value();
    index_img.move_index(3, 1);
    int offset = index_img.get_value();

    for (size_t i = 0; i < howmany; ++i) {
      score_img.move_index(0, offset + i);
      result.conservativeResize(result.rows() + 1);
      result(result.rows() - 1) = score_img.get_value();

      score_img.reset();
    }

    return result;

  }

  Eigen::Matrix<float, Eigen::Dynamic, 1> Reader::get_similar(const size_t index) const {
    Eigen::Matrix<float, Eigen::Dynamic, 1> result;
    
    // for thread safety
    Image<int> index_img (index_image);
    Image<int> sim_img (streamlines_image);

    // get howmany and offset
    index_img.move_index(0, index);
    size_t howmany = index_img.get_value();
    index_img.move_index(3, 1);
    int offset = index_img.get_value();

    for (size_t i = 0; i < howmany; ++i) {
      sim_img.move_index(0, offset + i);
      result.conservativeResize(result.rows() + 1);
      result(result.rows() - 1) = sim_img.get_value();

      sim_img.reset();
    }

    return result;
  }

}
