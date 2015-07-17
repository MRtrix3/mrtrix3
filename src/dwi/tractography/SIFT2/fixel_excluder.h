/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert Smith, 2014.

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


#ifndef __dwi_tractography_sift2_fixel_excluder_h__
#define __dwi_tractography_sift2_fixel_excluder_h__


#include <vector>

#include "bitset.h"

#include "dwi/tractography/SIFT/track_index_range.h"
#include "dwi/tractography/SIFT/types.h"


namespace MR {
namespace DWI {
namespace Tractography {
namespace SIFT2 {



class TckFactor;


class FixelExcluder
{

  public:
    FixelExcluder (TckFactor&, BitSet&);
    ~FixelExcluder();

    bool operator() (const SIFT::TrackIndexRange& range);


  private:
    TckFactor& master;
    BitSet& to_exclude;
    const double mu;

};






}
}
}
}



#endif

