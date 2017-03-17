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


#include <mutex>
#include <vector>

#include "command.h"
#include "progressbar.h"
#include "thread_queue.h"

#include "image.h"
#include "algo/loop.h"
#include "adapter/subset.h"

#include "connectome/connectome.h"

#include "surface/mesh.h"
#include "surface/mesh_multi.h"
#include "surface/algo/image2mesh.h"


using namespace MR;
using namespace App;
using namespace MR::Surface;


void usage ()
{

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Generate meshes from a label image";

  ARGUMENTS
  + Argument ("nodes_in", "the input node parcellation image").type_image_in()
  + Argument ("mesh_out", "the output mesh file").type_file_out();

  OPTIONS
  + Option ("blocky", "generate 'blocky' meshes with precise delineation of voxel edges, "
                      "rather than the default Marching Cubes approach");

}





void run ()
{

  Header labels_header = Header::open (argument[0]);
  Connectome::check (labels_header);
  auto labels = labels_header.get_image<uint32_t>();

  typedef Eigen::Array<int, 3, 1> voxel_corner_t;

  vector<voxel_corner_t> lower_corners, upper_corners;

  {
    for (auto i = Loop ("Importing label image", labels) (labels); i; ++i) {
      const uint32_t index = labels.value();
      if (index) {

        if (index >= lower_corners.size()) {
          lower_corners.resize (index+1, voxel_corner_t (labels.size(0), labels.size(1), labels.size(2)));
          upper_corners.resize (index+1, voxel_corner_t (-1, -1, -1));
        }

        for (size_t axis = 0; axis != 3; ++axis) {
          lower_corners[index][axis] = std::min (lower_corners[index][axis], int(labels.index (axis)));
          upper_corners[index][axis] = std::max (upper_corners[index][axis], int(labels.index (axis)));
        }

      }
    }
  }

  MeshMulti meshes (lower_corners.size(), MR::Surface::Mesh());
  meshes[0].set_name ("none");
  const bool blocky = get_options ("blocky").size();

  {
    std::mutex mutex;
    ProgressBar progress ("Generating meshes from labels", lower_corners.size() - 1);
    auto loader = [&] (size_t& out) { static size_t i = 1; out = i++; return (out != lower_corners.size()); };

    auto worker = [&] (const size_t& in)
    {
      vector<int> from, dimensions;
      for (size_t axis = 0; axis != 3; ++axis) {
        from.push_back (lower_corners[in][axis]);
        dimensions.push_back (upper_corners[in][axis] - lower_corners[in][axis] + 1);
      }
      Adapter::Subset<Image<uint32_t>> subset (labels, from, dimensions);

      auto scratch = Image<bool>::scratch (subset, "Node " + str(in) + " mask");
      for (auto i = Loop (subset) (subset, scratch); i; ++i)
        scratch.value() = (subset.value() == in);

      if (blocky)
        MR::Surface::Algo::image2mesh_blocky (scratch, meshes[in]);
      else
        MR::Surface::Algo::image2mesh_mc (scratch, meshes[in], 0.5);
      meshes[in].set_name (str(in));
      std::lock_guard<std::mutex> lock (mutex);
      ++progress;
      return true;
    };

    Thread::run_queue (loader, size_t(), Thread::multi (worker));
  }

  meshes.save (argument[1]);
}



