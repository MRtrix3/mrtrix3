#include "command.h"
#include "debug.h"
#include "image.h"

namespace MR {
}


using namespace MR;
using namespace App;


void usage ()
{
  AUTHOR = "Joe Bloggs (joe.bloggs@acme.org)";

  DESCRIPTION
  + "test vector adapter";

  ARGUMENTS
  + Argument ("in", "the input image.").type_image_in ()
  + Argument ("out", "the output image.").type_image_out ();
}


void run ()
{
  auto input = Image<float>::open (argument[0]).with_direct_io ();
  auto output = Image<float>::create (argument[1], input.header());
}
