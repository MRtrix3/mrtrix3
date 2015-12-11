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

 \page example_per_voxel_multithreaded_4D_processing Running a per-voxel operation on a 4D dataset in a multi-threaded loop

 This example computes the matrix multiplication of the vector of intensities for each voxel
 in the input dataset, producing a new dataset of the same size for the first 3
 (spatial) axes, and with the number of volumes specified by the user. In this
 example, we generate a matrix of random numbers for illustrative purposes.

 \code
#include "command.h"
#include "debug.h"
#include "image/buffer.h"
#include "image/voxel.h"
#include "image/loop.h"
#include "image/threaded_loop.h"
#include "math/matrix.h"
#include "math/vector.h"
#include "math/rng.h"

using namespace MR;
using namespace App;

// commmand-line description and syntax:
// (used to produce the help page and verify validity of arguments at runtime)

void usage ()
{
  AUTHOR = "Joe Bloggs (joe.bloggs@acme.org)";

  DESCRIPTION
  + "compute matrix multiplication of each voxel vector of "
    "values with matrix of random numbers";

  ARGUMENTS
  + Argument ("in", "the input image.").type_image_in ()
  + Argument ("out", "the output image.").type_image_out ();

  OPTIONS
  + Option ("size", "the number of rows of the matrix to be applied; "
                    "also the number of volumes in the output dataset (default = 10).")
  +   Argument ("num").type_integer(0, 10);
}



// Use typedefs as in previous example:
// I often use two such types: one for the type of the values stored in the
// dataset (I typically use float rather than double to keep RAM usage low),
// the other for the values used in the computation itself (this often
// needs to be higher precision to avoid rounding errors, etc).
typedef float value_type;
typedef float compute_type;



// In this case, we also declare a SharedInfo class to hold large data
// structures to be accessed read-only by each thread. This reduces the amount
// of RAM and especially CPU cache needed by the application at run-time, since
// this would otherwise be replicated for each thread (this quickly adds up on
// modern multi-core CPUs). 
class SharedInfo 
{
  public:
    // This creates the matrix to be applied, based on the number of volumes in
    // the input dataset, and those requested in the output dataset, and fills
    // it with random numbers from a normal distribution. 
    // Note that I often declare constructors as template functions if the
    // information from the input image is only needed within the body of the
    // constructor - i.e. no members are dependent on the type of the input
    // image - again, this means I don't need to worry about the type of the
    // image, the compiler will figure this out at compile-time.
    template <class InfoType>
      SharedInfo (const InfoType& info, size_t num_el) :
        A (num_el, info.dim(3)) {
          Math::RNG rng;
          for (size_t i = 0; i < A.rows(); ++i)
            for (size_t j = 0; j < A.columns(); ++j)
              A(i,j) = rng.normal();
        }

    Math::Matrix<compute_type> A;
};




// This is the functor that will be invoked per-voxel. 
// Note that here we pass the SharedInfo by const-reference, and construct a
// const member reference from it. At this point, the information from the
// SharedInfo will be accessible during processing, but only for const access
// (read-only), as desired - in general, we really don't want to modify the
// information in the SharedInfo during processing, as this will mess
// everything up for the other threads. 
class MathMulFunctor {
  public:

    // We pass the SharedInfo class to the constructor, and use it to allocate
    // thread-local vectors to hold the input and output values. Note that
    // these vector classes copy-construct appropriately (by deep copy), and
    // are therefore safe to use here with a default copy constructor. 
    MathMulFunctor (const SharedInfo& shared) :
      shared (shared),
      vec_in (shared.A.columns()), 
      vec_out (shared.A.rows()) { }

    // This is the actual operation to be performed. I use a templated
    // function, as per the previous example.
    template <class VoxeltypeIn, class VoxeltypeOut>
      void operator() (VoxeltypeIn& in, VoxeltypeOut& out)
      {
        Image::Loop loop (3);

        // read input values into vector:
        for (auto l = loop (in); l; ++l) 
          vec_in[in[3]] = in.value();

        // do matrix multiplication:
        Math::mult (vec_out, shared.A, vec_in);

        // write-back to output voxel:
        for (auto l = loop (out); l; ++l) 
          out.value() = vec_out[out[3]];
      }



  protected:
    const SharedInfo& shared;
    Math::Vector<compute_type> vec_in, vec_out;
};








void run ()
{
  // default value for number of output volumes:
  size_t nvol = 10;

  // check if -size option has been supplied, and update nvol accordingly:
  Options opt = get_options ("size");
  if (opt.size())
    nvol = opt[0][0];

  // create a Buffer to access the input data:
  Image::Buffer<value_type> buffer_in (argument[0]);

  // get the header of the input data, and modify to suit the output dataset:
  Image::Header header (buffer_in);
  header.datatype() = DataType::Float32;
  header.dim(3) = nvol;

  // create the output Buffer to store the output data, based on the updated
  // header information:
  Image::Buffer<value_type> buffer_out (argument[1], header);

  // Create the SharedInfo object:
  SharedInfo shared (buffer_in, nvol);

  // create a threaded loop object that will display a progress message, and
  // iterate over buffer_in in order of increasing stride. In this case, only loop
  // over the first 3 axes (see Image::ThreadedLoop documentation for details): 
  Image::ThreadedLoop loop ("performing matrix multiplication", buffer_in, 0, 3);

  // run the loop, invoking the functor MathMulFunctor that you constructed:
  loop.run (MathMulFunctor (shared), buffer_in.voxel(), buffer_out.voxel());
}
\endcode

A few tips on how to use the above structure:

- Use the SharedInfo class to hold any read-only data needed by each thread
during execution, and perform all the initialisation required in this class.
This helps to keep each thread light-weight by minimising both the amount of
RAM each thread needs, and the amount of work each thread needs to do.

- If you need each thread to use data types that imply dynamic allocation (e.g.
  vectors, matrices, etc), declare them as member variables, and if possible
  allocate them once in the constructor. If these are declared in the body of the
  operator() method, this will probably result in reduced performance due to the
  cost of allocation/deallocation, as well as the synchronisation calls the
  memory management infrastructure will need to make to keep its internal data
  structure in order in a multi-threaded environment. 

- temporary variables that do not imply dynamic allocation can be declared
within the body of the operator() method, since these will be allocated on the
stack and don't incur a run-time cost.

*/

}



