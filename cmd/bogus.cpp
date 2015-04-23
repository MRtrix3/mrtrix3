#include "command.h"
#include "debug.h"
#include "image.h"
#include "timer.h"
#include "algo/loop.h"
#include "algo/threaded_loop.h"

using namespace MR;
using namespace App;


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

typedef float value_type;

void run ()
{
  value_type power = 2.0;
  auto opt = get_options ("power");
  if (opt.size())
    power = opt[0][0];

  auto in = Header::open (argument[0]).get_image<value_type>().with_direct_io ();
  auto header = in.header();
  header.datatype() = DataType::Float32;
  auto out = Header::create (argument[1], header).get_image<value_type>(); 

  Timer timer;

  for (auto l = LoopInOrder (in).run (in, out); l; ++l)
    out.value() = std::pow (in.value(), power);
  CONSOLE ("single-threaded loop: " + str(timer.elapsed(), 6) + "s");

  timer.start();
  ThreadedLoop (in).run ([&](decltype(in)& vin, decltype(out)& vout) {
      vout.value() = std::pow (vin.value(), power);
      }, in, out);
  CONSOLE ("multi-threaded loop: " + str(timer.elapsed(), 6) + "s");
}

