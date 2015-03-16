/*
   Copyright 2014 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier

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

#error - this file is for documentation purposes only!
#error - It should NOT be included in other code files.

namespace MR
{

  /*! 
 \page example_per_datum_multithreaded_processing Running a per-datum operation in a multi-threaded loop

 This example simply computes the exponential of the intensity for each data point
 in the input dataset, producing a new dataset of the same size.

 \code
#include "command.h"
#include "debug.h"
#include "image/buffer.h"
#include "image/voxel.h"
#include "image/threaded_loop.h"

using namespace MR;
using namespace App;

// commmand-line description and syntax:
// (used to produce the help page and verify validity of arguments at runtime)

void usage ()
{
  AUTHOR = "Joe Bloggs (joe.bloggs@acme.org)";

  DESCRIPTION
  + "compute exponential of each voxel intensity";

  ARGUMENTS
  + Argument ("in", "the input image.").type_image_in ()
  + Argument ("out", "the output image.").type_image_out ();

  OPTIONS
  + Option ("lambda", "the exponent scale factor lambda in the equation 'exp (lambda * intensity)'")
  +   Argument ("value").type_float();
}



// It is a good idea to use typedef's to help with flexibility if types need to
// be changed later on.
typedef float value_type;

// This is the functor that will be invoked per-voxel. Note this could be a
// simple function if the operation to be performed was independent of any
// parameters - here we use a functor class to hold the lambda multiplier as a
// member variable.
class ExpFunctor {
  public:
    ExpFunctor (value_type lambda) :
     lambda (lambda) { }

    // This is the actual operation to be performed. 
    // I typically declare this as a templated function on the input
    // VoxelTypes, so that I don't need to worry about what their actual types
    // might be - the compiler will figure this out at compile-time.
    template <class VoxeltypeIn, class VoxeltypeOut>
      void operator() (const VoxeltypeIn& in, VoxeltypeOut& out)
      {
        out.value() = std::exp (lambda * in.value());
      }

  protected:
    const value_type lambda;
};





// This is where execution proper starts - the equivalent of main(). 
// The difference is that this is invoked after all command-line parsing has
// been done.

void run ()
{
  // default value for lambda:
  value_type lambda = 1.0;

  // check if -lambda option has been supplied, and update lambda accordingly
  // if so:
  Options opt = get_options ("lambda");
  if (opt.size())
    lambda = opt[0][0];

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

  // create a threaded loop object that will display a progress message, and
  // iterate over buffer_in in order of increasing stride. It's typically best
  // to make sure the loop iterates over the input dataset in RAM-contiguous
  // order (increasing stride - the default) since this maximises memory access
  // throughput.
  Image::ThreadedLoop loop ("computing exponential...", vox_in);

  // run the loop, invoking the functor ExpFunctor that you constructed with
  // the user-supplied lambda value (or default of 1), using vox_in as the
  // first argument and vox_out as the second:
  loop.run (ExpFunctor (lambda), vox_in, vox_out);


  // note that for simple operations, it is also possible to use lambda
  // functions, avoiding the need to declare a full-blown functor class:
  loop.run ([&] (decltype(vox_in)& in, decltype(vox_out)& out) {
        out.value() = std::exp (lambda * in.value());
        }, vox_in, vox_out);
}
\endcode

It is worth re-iterating that the final run() call on the threaded loop does
quite a bit of work behind the scenes. The main issue is that it will create
copies of the functor you provided using its copy-constructor, and call each of
these copies' operator() method within its own thread. This means you need to
ensure that your functor's copy constructor behaves appropriately - each copy
must be able to operate independently without affecting any of the other
copies. Pay special attention to any data accessed via a member pointer or
reference - by default, these are copy-constructed by value, so that each
functor still points to the same object - this is fine as long as this object
is \e never written to. If this what you intended, declare these const to
minimise the chances of unintentional write access (see next example for
details).

*/

}



