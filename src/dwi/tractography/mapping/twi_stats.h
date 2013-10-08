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


enum contrast_t { TDI, PRECISE_TDI, ENDPOINT, LENGTH, INVLENGTH, SCALAR_MAP, SCALAR_MAP_COUNT, FOD_AMP, CURVATURE };
enum vox_stat_t { V_SUM, V_MIN, V_MEAN, V_MAX };
enum tck_stat_t { T_SUM, T_MIN, T_MEAN, T_MAX, T_MEDIAN, T_MEAN_NONZERO, GAUSSIAN, ENDS_MIN, ENDS_MEAN, ENDS_MAX, ENDS_PROD, ENDS_CORR };

extern const char* contrasts[];
extern const char* voxel_statistics[];
extern const char* track_statistics[];


}
}
}
}

#endif



