#include "command.h"
#include "debug.h"
#include "image.h"
#include "timer.h"
#include "math/rng.h"
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
  + Option ("power", "the power by which to raise each value (default: 1)")
  +   Argument ("value").type_float()

  + Option ("noise", "the std. dev. of the noise to add to each value (default: 1)")
  +   Argument ("value").type_float();
}

typedef float value_type;


void run ()
{
  value_type power = 1.0;
  auto opt = get_options ("power");
  if (opt.size())
    power = opt[0][0];

  value_type noise = 1.0;
  opt = get_options ("noise");
  if (opt.size())
    noise = opt[0][0];

  auto in = Image<value_type>::open (argument[0]).with_direct_io ();
  auto header = in.header();

  auto scratch = Image<value_type>::scratch (header);
  struct noisify {
    const value_type noise;
    Math::RNG::Normal<value_type> rng;
    void operator() (decltype(in)& in, decltype(scratch)& out) { out.value() = in.value() + noise*rng(); }
  };
  Timer timer;
  ThreadedLoop ("adding noise...", in).run (noisify({noise}), in, scratch);
  CONSOLE ("time taken: " + str(timer.elapsed(), 6) + "s");


  header.datatype() = DataType::Float32;
  auto out = Image<value_type>::create (argument[1], header).with_direct_io(); 

  timer.start();
  ThreadedLoop ("raising to power " + str(power) + "...", scratch).run ([&](decltype(in)& in, decltype(out)& out) {
      out.value() = std::pow (in.value(), power);
      }, scratch, out);
  CONSOLE ("time taken: " + str(timer.elapsed(), 6) + "s");
}

