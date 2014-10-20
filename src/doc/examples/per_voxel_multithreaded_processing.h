/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 16/08/09.

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
typedef float value_type;
typedef float compute_type;
typedef Image::Buffer<value_type> BufferIn;
typedef Image::Buffer<value_type> BufferOut;



// In this case, we also declare a SharedInfo class to hold large data
// structures to be accessed read-only by each thread. This keeps the amount of
// RAM and especially CPU cache needed by the application at run-time, since
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
    // image, the compile will figure this out at compile-time.
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
        for (loop.start (in); loop.ok(); loop.next (in)) 
          vec_in[in[3]] = in.value();

        // do matrix multiplication:
        Math::mult (vec_out, shared.A, vec_in);

        // write-back to output voxel:
        for (loop.start (out); loop.ok(); loop.next (out)) 
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
  BufferIn buffer_in (argument[0]);

  // get the header of the input data, and modify to suit the output dataset:
  Image::Header header (buffer_in);
  header.datatype() = DataType::Float32;
  header.dim(3) = nvol;

  // create the output Buffer to store the output data, based on the updated
  // header information:
  BufferOut buffer_out (argument[1], header);

  // create the appropriate Voxel objects to access the intensities themselves:
  BufferIn::voxel_type vox_in (buffer_in);
  BufferIn::voxel_type vox_out (buffer_out);

  // Create the SharedInfo object:
  SharedInfo shared (vox_in, nvol);

  // create a threaded loop object that will display a progress message, and
  // iterate over vox_in in order of increasing stride. In this case, only loop
  // over the first 3 axes, with one axis run within each thread between
  // synchronisation calls (see Image::ThreadedLoop documentation for details). 
  Image::ThreadedLoop loop ("performing matrix multiplication...", vox_in, 1, 0, 3);

  // run the loop, invoking the functor MathMulFunctor that you constructed
  loop.run (MathMulFunctor (shared), vox_in, vox_out);
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



