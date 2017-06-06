/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#ifndef __dwi_tractography_mapping_twi_stat_h__
#define __dwi_tractography_mapping_twi_stat_h__


namespace MR {
namespace DWI {
namespace Tractography {
namespace Mapping {


enum contrast_t { TDI, LENGTH, INVLENGTH, SCALAR_MAP, SCALAR_MAP_COUNT, FOD_AMP, CURVATURE, VECTOR_FILE };
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



