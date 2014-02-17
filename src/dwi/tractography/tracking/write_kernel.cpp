#include "dwi/tractography/tracking/write_kernel.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Tracking
      {


          bool WriteKernel::operator() (const GeneratedTrack& tck)
          {
            if (complete())
              return false;
            if (tck.size() && seeds) {
              const Point<float>& p = tck[tck.get_seed_index()];
              (*seeds) << str(writer.count) << "," << str(tck.get_seed_index()) << "," << str(p[0]) << "," << str(p[1]) << "," << str(p[2]) << ",\n";
            }
            writer (tck);
            if (timer && App::log_level > 0) {
              fprintf (stderr, "\r%8zu generated, %8zu selected    [%3d%%]",
                  writer.total_count, writer.count,
                  (int(100.0 * std::max (writer.total_count/float(S.max_num_attempts), writer.count/float(S.max_num_tracks)))));
            }
            return true;
          }

          bool WriteKernel::operator() (const GeneratedTrack& in, Tractography::Streamline<>& out)
          {
            out.index = writer.count;
            out.weight = 1.0;
            if (!operator() (in))
              return false;
            out = in;
            return true;
          }




      }
    }
  }
}

