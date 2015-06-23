/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 2015.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

*/


#include <vector>

#include "command.h"
#include "progressbar.h"
#include "thread_queue.h"

#include "image/buffer.h"
#include "image/buffer_scratch.h"
#include "image/loop.h"
#include "image/nav.h"
#include "image/voxel.h"
#include "image/adapter/subset.h"

#include "mesh/mesh.h"
#include "mesh/vox2mesh.h"


using namespace MR;
using namespace App;
using namespace MR::Mesh;


void usage ()
{

	AUTHOR = "Robert E. Smith (r.smith@brain.org.au)";

  DESCRIPTION
  + "generate meshes from a label image.";

  ARGUMENTS
  + Argument ("nodes_in", "the input node parcellation image").type_image_in()
  + Argument ("mesh_out", "the output mesh file").type_file_out();

  OPTIONS
  + Option ("blocky", "generate 'blocky' meshes with precise delineation of voxel edges, "
                      "rather than the default Marching Cubes approach");

};





void run ()
{

  Image::Header H (argument[0]);
  if (!H.datatype().is_integer())
    throw Exception ("Input image must have an integer data type");
  Image::Buffer<uint32_t> label_data (H);
  auto labels = label_data.voxel();

  std::vector< Point<int> > lower_corners, upper_corners;

  {
    Image::LoopInOrder loop (labels, "Importing label image... ");
    for (auto i = loop (labels); i; ++i) {
      const size_t index = labels.value();
      if (index) {

        if (index >= lower_corners.size()) {
          lower_corners.resize (index+1, Point<int> (H.dim(0), H.dim(1), H.dim(2)));
          upper_corners.resize (index+1, Point<int> (-1, -1, -1));
        }

        for (size_t axis = 0; axis != 3; ++axis) {
          lower_corners[index][axis] = std::min (lower_corners[index][axis], int(labels[axis]));
          upper_corners[index][axis] = std::max (upper_corners[index][axis], int(labels[axis]));
        }

      }
    }
  }

  MeshMulti meshes (lower_corners.size(), Mesh::Mesh());
  meshes[0].set_name ("none");

  {
    std::mutex mutex;
    ProgressBar progress ("Generating meshes from labels... ", lower_corners.size() - 1);
    const bool blocky = get_options ("blocky").size();
    auto loader = [&] (size_t& out) { static size_t i = 1; out = i++; return (out != lower_corners.size()); };

    auto worker = [&] (const size_t& in)
    {
      MR::Image::Adapter::Subset<decltype(labels)> subset (labels, lower_corners[in], upper_corners[in] - lower_corners[in] + Point<int> (1, 1, 1));

      MR::Image::BufferScratch<bool> buffer (subset.info(), "Node " + str(in) + " mask");
      auto voxel = buffer.voxel();
      Image::LoopInOrder loop (subset.info());
      for (auto i = loop (subset, voxel); i; ++i)
        voxel.value() = (subset.value() == in);

      if (blocky)
        MR::Mesh::vox2mesh (voxel, meshes[in]);
      else
        MR::Mesh::vox2mesh_mc (voxel, 0.5, meshes[in]);
      meshes[in].transform_voxel_to_realspace (subset.info());
      meshes[in].set_name (str(in));
      std::lock_guard<std::mutex> lock (mutex);
      ++progress;
      return true;
    };

    Thread::run_queue (loader, size_t(), Thread::multi (worker));
  }

  meshes.save (argument[1]);
}



