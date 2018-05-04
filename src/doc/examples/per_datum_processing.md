Running a per-datum operation (without multithreading)     {#example_per_datum_processing}
======================================================

This example simply computes the exponential of the intensity for each data point
in the input dataset, producing a new dataset of the same size.

~~~{.cpp}
#include "command.h"
#include "image.h"
#include "algo/loop.h"

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
using value_type = float;


// This is where execution proper starts - the equivalent of main(). 
// The difference is that this is invoked after all command-line parsing has
// been done.
void run ()
{
  // get power from command-line if supplied, default to 2.0:
  value_type power = get_option_value ("power", 2.0);

  // Image to access the input data:
  auto in = Image<value_type>::open (argument[0]);

  // get the header of the input data, and modify to suit the output dataset:
  Header header (in);
  header.datatype() = DataType::Float32;

  // create the output Buffer to store the output data, based on the updated
  // header information:
  auto out = Image<value_type>::create (argument[1], header);

  // create the loop structure. This version will traverse the image data in
  // order of increasing stride of the input dataset, to ensure contiguous
  // voxel values are most likely to be processed consecutively. This helps to
  // ensure maximum performance. 
  //
  // Note that we haven't specified any axes, so this will process datasets of
  // any dimensionality, whether 3D, 4D or ND:
  auto loop = Loop (in);

  // run the loop:
  for (auto l = loop (in, out); l; ++l)
    out.value() = std::pow (in.value(), power);

  // That's it! Data write-back is performed by the Image destructor,
  // invoked when it goes out of scope at function exit.
}
~~~

