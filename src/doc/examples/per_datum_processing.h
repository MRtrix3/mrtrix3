/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#error - this file is for documentation purposes only!
#error - It should NOT be included in other code files.

namespace MR
{

  /*! 
 \page example_per_datum_processing Running a per-datum operation (without multithreading)

 This example simply computes the exponential of the intensity for each data point
 in the input dataset, producing a new dataset of the same size.

 \code
#include "command.h"
#include "debug.h"
#include "image/buffer.h"
#include "image/voxel.h"
#include "image/loop.h"

using namespace MR;
using namespace App;

// commmand-line description and syntax:
// (used to produce the help page and verify validity of arguments at runtime)

void usage ()
{
  AUTHOR = "Joe Bloggs (joe.bloggs@acme.org)";

  DESCRIPTION
  + "raise each voxel intensity to the given power (default: 2)";

  ARGUMENTS
  + Argument ("in", "the input image.").type_image_in ()
  + Argument ("out", "the output image.").type_image_out ();

  OPTIONS
  + Option ("power", "the power by which to raise each value (default: 2)")
  +   Argument ("value").type_float();
}



// It is a good idea to use typedef's to help with flexibility if types need to
// be changed later on.
typedef float value_type;


// This is where execution proper starts - the equivalent of main(). 
// The difference is that this is invoked after all command-line parsing has
// been done.

void run ()
{
  // default value for power:
  value_type power = 2.0;

  // check if -power option has been supplied, and update power accordingly
  // if so:
  Options opt = get_options ("power");
  if (opt.size())
    power = opt[0][0];

  // create a Buffer to access the input data:
  Image::Buffer<value_type> buffer_in (argument[0]);

  // get the header of the input data, and modify to suit the output dataset:
  Image::Header header (buffer_in);
  header.datatype() = DataType::Float32;

  // create the output Buffer to store the output data, based on the updated
  // header information:
  Image::Buffer<value_type> buffer_out (argument[1], header);

  // create the appropriate Voxel objects to access the intensities themselves:
  auto vox_in = buffer_in.voxel();
  auto vox_out = buffer_out.voxel();

  // create the loop structure. This version will traverse the image data in
  // order of increasing stride of the input dataset, to ensure contiguous
  // voxel values are most likely to be processed consecutively. This helps to
  // ensure maximum performance. 
  // Note that we haven't specified any axes, so this will process datasets of
  // any dimensionality, whether 3D, 4D or ND:
  Image::LoopInOrder loop (vox_in);

  // run the loop:
  for (auto l = loop (vox_in, vox_out); l; ++l)
    vox_out.value() = std::pow (vox_in.value(), power);

  // That's it! Data write-back is performed by the Image::Buffer destructor,
  // invoked when it goes out of scope at function exit.
}
\endcode

*/

}



