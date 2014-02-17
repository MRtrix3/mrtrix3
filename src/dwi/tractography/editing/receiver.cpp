/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


#include "dwi/tractography/editing/receiver.h"


namespace MR {
namespace DWI {
namespace Tractography {
namespace Editing {





bool Receiver::operator() (const Tractography::Streamline<>& in)
{

  if (number && (count == number))
    return false;

  ++total_count;

  if (in.empty()) {
    writer (in);
    update_cmdline();
    return true;
  }

  if (in[0].valid()) {

    if (skip) {
      --skip;
      update_cmdline();
      return true;
    }
    writer (in);

  } else {

    // Explicitly handle case where the streamline has been cropped into multiple components
    // Worker class separates track segments using invalid points as delimiters
    Tractography::Streamline<> temp;
    temp.index = in.index;
    temp.weight = in.weight;
    for (Tractography::Streamline<>::const_iterator p = in.begin(); p != in.end(); ++p) {
      if (p->valid()) {
        temp.push_back (*p);
      } else if (temp.size()) {
        writer (temp);
        temp.clear();
      }
    }

  }

  ++count;
  update_cmdline();
  return (!(number && (count == number)));

}



void Receiver::update_cmdline()
{
  if (timer && App::log_level > 0)
    print();
}



void Receiver::print()
{
  const float input_fraction = in_count ? (total_count / float(in_count)) : 0.0;
  const float output_fraction = number ? (count / float(number)) : 0.0;
  fprintf (stderr, "\r%8zu read, %8zu written    [%3d%%]",
           total_count, count, int(100.0 * std::max(input_fraction, output_fraction)));
}






}
}
}
}

