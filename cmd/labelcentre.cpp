/* Copyright (c) 2008-2018 the MRtrix3 contributors.
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


#include "command.h"
#include "header.h"
#include "image.h"
#include "image_helpers.h"
#include "transform.h"
#include "types.h"

#include "connectome/connectome.h"

using namespace MR;
using namespace App;


const char* space_options[] = { "scanner", "voxel", nullptr };


void usage ()
{
  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Determine the centre of mass / centre of gravity of each parcel within a label image";

  ARGUMENTS
  + Argument ("input", "the input label image").type_image_in()
  + Argument ("space", "the coordinate space in which to provide the centres (options are: " + join (space_options, ",") + ")").type_choice (space_options);

}


using Connectome::node_t;
using vector_type = Eigen::Array<default_type, Eigen::Dynamic, 1>;
using matrix_type = Eigen::Matrix<default_type, Eigen::Dynamic, 3>;


void run ()
{
  Header H = Header::open (argument[0]);
  if (H.ndim() > 3)
    throw Exception ("Command does not accept images with more than 3 dimensions");
  Connectome::check (H);
  Image<node_t> image = H.get_image<node_t>();
  
  matrix_type coms;
  vector_type masses;

  for (auto l = Loop(image) (image); l; ++l) {
    const node_t value = image.value();
    if (value) {
      if (value > coms.rows()) {
        coms.conservativeResizeLike (matrix_type::Zero (value, 3));
        masses.conservativeResizeLike (vector_type::Zero (value));
      }
      coms.row(value-1) += Eigen::Vector3 (image.index(0), image.index(1), image.index(2));
      masses[value-1]++;
    }
  }

  coms = coms.array().colwise() / masses;
  
  if (!int(argument[1]))
    coms = (image.transform() * coms.transpose()).transpose();

  std::cout << coms;
}