/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 2011.

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

#ifndef __dwi_tractography_mapping_twi_stat_h__
#define __dwi_tractography_mapping_twi_stat_h__


namespace MR {
namespace DWI {
namespace Tractography {
namespace Mapping {


enum stat_t { SUM, MIN, MEAN, MEDIAN, MAX, GAUSSIAN, FMRI_MIN, FMRI_MEAN, FMRI_MAX };
const char* statistics[] = { "sum", "min", "mean", "median", "max", "gaussian", "fmri_min", "fmri_mean", "fmri_max", NULL };


}
}
}
}

#endif



