Running a per-datum operation in a multi-threaded loop     {#example_per_datum_multithreaded_processing}
======================================================

This example simply computes the exponential of the intensity for each data point
in the input dataset, producing a new dataset of the same size.

~~~{.cpp}
#include "command.h"
#include "image.h"
#include "algo/threaded_loop.h"

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
using value_type = float;

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
  // get power from command-line if supplied, default to 2.0:
  value_type lambda = get_option_value ("lambda", 1.0);

  // Image to access the input data:
  auto in = Image<value_type>::open (argument[0]);

  // get the header of the input data, and modify to suit the output dataset:
  Header header (in);
  header.datatype() = DataType::Float32;

  // create the output Buffer to store the output data, based on the updated
  // header information:
  auto out = Image<value_type>::create (argument[1], header);

  // create a threaded loop object that will display a progress message, and
  // iterate over buffer_in in order of increasing stride. It's typically best
  // to make sure the loop iterates over the input dataset in RAM-contiguous
  // order (increasing stride - the default) since this maximises memory access
  // throughput.
  auto loop = ThreadedLoop ("computing exponential", in);

  // run the loop, invoking the functor ExpFunctor that you constructed with
  // the user-supplied lambda value (or default of 1), using vox_in as the
  // first argument and vox_out as the second:
  loop.run (ExpFunctor (lambda), in, out);


  // note that for simple operations, it is also possible to use lambda
  // functions, avoiding the need to declare a full-blown functor class:
  loop.run ([&] (decltype(in)& vin, decltype(out)& vout) {
        vout.value() = std::exp (lambda * vin.value());
        }, in, out);
}
~~~

It is worth re-iterating that the final `run()` call on the threaded loop does
quite a bit of work behind the scenes. The main issue is that it will create
copies of the functor you provided using its copy-constructor, and call each of
these copies' `operator()` method within its own thread. This means you need to
ensure that your functor's copy constructor behaves appropriately - each copy
must be able to operate independently without affecting any of the other
copies. Pay special attention to any data accessed via a member pointer or
reference - by default, these are copy-constructed by value, so that each
functor still points to the same object - this is fine as long as this object
is _never_ written to. If this what you intended, declare these `const` to
minimise the chances of unintentional write access (see next example for
details).

