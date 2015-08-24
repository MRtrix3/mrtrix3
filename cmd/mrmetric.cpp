#include "command.h"
#include "image.h"
#include "algo/loop.h"

using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  DESCRIPTION
  + "computes a similarity metric between two images. Currently only the mean squared difference is implemented";

  ARGUMENTS
  + Argument ("image1", "the first input image.").type_image_in ()
  + Argument ("image2", "the second input image.").type_image_out ();

}

typedef float value_type;



void run ()
{
  auto input1 = Image<float>::open (argument[0]);
  auto input2 = Image<float>::open (argument[1]);

  check_dimensions (input1, input2);

  default_type sos = 0.0;
  for (auto i = Loop() (input1, input2); i ;++i)
    sos += std::pow (input1.value() - input2.value(), 2);
  CONSOLE ("MSD: " + str(sos / static_cast<default_type>(input1.size(0) * input1.size(1) * input1.size(2))));
}

