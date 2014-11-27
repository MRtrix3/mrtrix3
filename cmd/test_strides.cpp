#include "command.h"
#include "debug.h"
#include "image/stride.h"
#include "image/info.h"

using namespace MR;
using namespace App;

void usage ()
{
  DESCRIPTION
    + "test Image::stride::sanitise()";
  ARGUMENTS
    + Argument ("in", "the input strides.").type_sequence_int ()
    + Argument ("out", "the reference strides.").type_sequence_int ();
}


Image::Stride::List convert (const std::vector<int>& in) {
  Image::Stride::List out; 
  for (size_t n = 0; n < in.size(); ++n)
    out.push_back (in[n]); 
  return out; 
};

void run ()
{
  std::vector<int> istrides = argument[0];
  std::vector<int> idesired = argument[1];

  Image::Stride::List strides = convert (istrides);
  Image::Stride::List desired = convert (idesired);

  Image::Info info;
  info.set_ndim (strides.size());
  Image::Stride::set (info, strides);

  VAR (Image::Stride::get_nearest_match (info, desired));
}
