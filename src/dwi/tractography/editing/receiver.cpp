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

