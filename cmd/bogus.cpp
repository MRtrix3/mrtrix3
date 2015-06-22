#include "command.h"
#include "debug.h"
#include "thread.h"
#include "dwi/tractography/rng.h"

using namespace MR;
using namespace App;


std::mutex mutex;

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

using namespace DWI::Tractography;

struct thread_func {
  void execute () {
    std::lock_guard<std::mutex> lock (mutex);
    std::cerr << &rng << ": " << rng() << " " << rng() << " " << rng() << "\n";
  }
};

void run ()
{
  std::cerr << &rng << ": " << rng() << " " << rng() << " " << rng() << "\n";
  Thread::run (Thread::multi (thread_func()));
}

