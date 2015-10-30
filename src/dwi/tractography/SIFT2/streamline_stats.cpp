/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert Smith, 2013.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

 */


#include "dwi/tractography/SIFT2/streamline_stats.h"



namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace SIFT2 {




      StreamlineStats::StreamlineStats() :
          min (std::numeric_limits<double>::infinity()),
          max (-std::numeric_limits<double>::infinity()),
          mean (0.0),
          mean_abs (0.0),
          var (0.0),
          count (0),
          nonzero (0) { }


      StreamlineStats::StreamlineStats (const StreamlineStats& that) :
          min (std::numeric_limits<double>::infinity()),
          max (-std::numeric_limits<double>::infinity()),
          mean (0.0),
          mean_abs (0.0),
          var (0.0),
          count (0),
          nonzero (0) { }



      StreamlineStats& StreamlineStats::operator+= (const double i)
      {
        min = std::min (min, i);
        max = std::max (max, i);
        mean += i;
        mean_abs += std::abs (i);
        var += Math::pow2 (i);
        ++count;
        if (i)
          ++nonzero;
        return *this;
      }



      StreamlineStats& StreamlineStats::operator+= (const StreamlineStats& i)
      {
        min = std::min (min, i.min);
        max = std::max (max, i.max);
        mean += i.mean;
        mean_abs += i.mean_abs;
        var += i.var;
        count += i.count;
        nonzero += i.nonzero;
        return *this;
      }



      void StreamlineStats::normalise()
      {
        assert (count);
        mean /= double(count);
        mean_abs /= double(count);
        var /= double(count-1);
      }



      }
    }
  }
}



