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



