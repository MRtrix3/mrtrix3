#include "command.h"
#include "debug.h"
#include "image.h"

#include "adapter/vector.h"

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

  Adapter::Vector<decltype(input)> vector_input (input);

  vector_input.index(0) = 78;
  vector_input.index(1) = 57;
  vector_input.index(2) = 39;

  std::cout << vector_input.value() << std::endl << std::endl;

  Eigen::VectorXf test = vector_input.value();
  std::cout << test << std::endl << std::endl;
  test[1] = 0.5;

  vector_input.value() = test; // seg faults
  std::cout << vector_input.value() << std::endl << std::endl;

  vector_input.value()[1] = 10;  // seg faults
  std::cout << vector_input.value() << std::endl;

  auto temp = Image<float>::scratch (input.header());

  Adapter::Vector<decltype(temp)> vector_temp (temp);

  threaded_copy (vector_input, vector_temp, 0, 3);

  auto output = Image<float>::create (argument[1], input.header());
  copy (temp, output);
}
