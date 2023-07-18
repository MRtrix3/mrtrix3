/* Copyright (c) 2008-2023 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
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
// Note: ENDS_CORR not provided as a command-line option
enum tck_stat_t { T_SUM, T_MIN, T_MEAN, T_MAX, T_MEDIAN, T_MEAN_NONZERO, GAUSSIAN, ENDS_MIN, ENDS_MEAN, ENDS_MAX, ENDS_PROD, ENDS_CORR };

extern const char* contrasts[];
extern const char* voxel_statistics[];
extern const char* track_statistics[];


}
}
}
}

#endif



