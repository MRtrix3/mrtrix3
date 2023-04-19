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

#include "command.h"
#include "image.h"
#include "interp/cubic.h"
#include "interp/linear.h"
#include "interp/nearest.h"
#include "interp/sinc.h"
#include "surface/mesh.h"



using namespace MR;
using namespace App;
using namespace MR::Interp;
using MR::Surface::Mesh;


using vector_type = Eigen::Array<float, Eigen::Dynamic, 1>;


// TODO This could be centralised and shared across multiple commands
const char* interp_choices[] = { "nearest", "linear", "cubic", "sinc", nullptr };


void usage ()
{

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Sample the values of an image the vertex locations of a surface mesh";

  DESCRIPTION
  + "This command assumes that the surface is defined in such a way that the "
    "vertices are defined in real / scanner space. If a surface is defined with respect "
    "to some other space, it is necessary to first explicitly perform a spatial "
    "transformation of the surface data prior to running this command.";

  ARGUMENTS
  + Argument ("input",  "the input mesh file").type_file_in()
  + Argument ("input",  "the input image").type_image_in()
  + Argument ("output", "the output sampled values").type_file_out();

  OPTIONS
  + Option ("interp",
        "set the interpolation method to use when reslicing (choices: nearest, linear, cubic, sinc. Default: cubic).")
    + Argument ("method").type_choice (interp_choices);

}



template <class InterpType>
vector_type sample (const Mesh& mesh, const Image<float>& image)
{
  InterpType interp (image);
  vector_type result = vector_type::Zero (mesh.num_vertices());
  for (size_t i = 0; i != mesh.num_vertices(); ++i) {
    if (interp.scanner (mesh.vert (i)))
      result[i] = interp.value();
    else
      result[i] = std::numeric_limits<float>::quiet_NaN();
  }
  return result;
}



void run ()
{

  const Mesh mesh (argument[0]);
  Image<float> image = Image<float>::open (argument[1]);
  auto opt = get_options ("interp");
  const size_t interp_type = opt.size() ? opt[0][0] : 2;
  vector_type data;
  switch (interp_type) {
    case 0: data = sample<Interp::Nearest<Image<float>>> (mesh, image); break;
    case 1: data = sample<Interp::Linear <Image<float>>> (mesh, image); break;
    case 2: data = sample<Interp::Cubic  <Image<float>>> (mesh, image); break;
    case 3: data = sample<Interp::Sinc   <Image<float>>> (mesh, image); break;
    default: assert (false);
  }
  MR::save_vector (data, argument[2]);

}
