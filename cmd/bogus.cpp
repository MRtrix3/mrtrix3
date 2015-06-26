#include "command.h"
#include "debug.h"
#include "thread.h"
#include "dwi/tractography/rng.h"
#include "image.h"

#include "adapter/median3D.h"

namespace MR {
}












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

//struct thread_func {
//  void execute () {
//    std::lock_guard<std::mutex> lock (mutex);
//    std::cerr << &rng << ": " << rng() << " " << rng() << " " << rng() << "\n";
//  }
//};


void run ()
{

  auto input = Image<float>::open (argument[0]).with_direct_io();
  auto output = Image<float>::create (argument[1], input);

  VAR (is_header_type<decltype(input)>::value);
  VAR (is_image_type<decltype(input)>::value);
  VAR (is_pure_image<decltype(input)>::value);
  VAR (is_adapter_type<decltype(input)>::value);

  auto adapter = Adapter::make <Adapter::Median3D> (input);

  VAR (is_header_type<decltype(adapter)>::value);
  VAR (is_image_type<decltype(adapter)>::value);
  VAR (is_pure_image<decltype(adapter)>::value);
  VAR (is_adapter_type<decltype(adapter)>::value);

  save (input, "out.mih");
  save (adapter, "out2.mif");

  display (adapter);

  input.index(0) = 10;
  input.index(1) = 13;
  input.index(2) = 10;

  output.index(0) = input.index(0);
  output.index(1) = input.index(1);
  output.index(2) = input.index(2);


  VAR (input.value());
  VAR (output.value());

  output.value() = input.value();

  VAR (input);
  VAR (output);
  VAR (input.value());
  VAR (output.value());


//  std::cerr << &rng << ": " << rng() << " " << rng() << " " << rng() << "\n";
//  Thread::run (Thread::multi (thread_func()));
}

