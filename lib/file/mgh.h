/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see www.mrtrix.org
 *
 */

#ifndef _MGH_HEADER_
#define _MGH_HEADER_


#include <vector>
#include <string>

#include "types.h"

/*=================*/
#ifdef  __cplusplus
extern "C" {
#endif
  /*=================*/

// Necessary because the 16-bit field goodRASFlag throws the subsequent floats off the 32-bit word block addresses
#pragma pack(push, 1) // exact fit - no padding

  /*! \struct mgh_header
      \brief Data structure defining the fields in the mgh header.
             This binary header should be found at the beginning of a valid
             mgh header file.
             http://surfer.nmr.mgh.harvard.edu/fswiki/FsTutorial/MghFormat
   */
  /*************************/  /************************/
  struct mgh_header {
    /*************************/  /************************/

    int32_t version;       /*!< MUST be 1             */
    int32_t width;         /*!< first dimension of the image buffer (fastest) */
    int32_t height;        /*!< second dimension of the image buffer (2nd fastest) */
    int32_t depth;         /*!< slowest dimension when reading the buffer */
    int32_t nframes;       /*!< number of scalar components contained in the buffer (i.e. number of components per voxel) */
    int32_t type;          /*!< data type of the image buffer; can be one of the following: UCHAR, SHORT, INT, or FLOAT (specified as 0, 4, 1, or 3, respectively) */
    int32_t dof;           /*!< degrees of freedom (where appropriate) */
    int16_t goodRASFlag;   /*!< if true, the direction cosines, which will be detailed next, are in the header; if false, a CORONAL orientation will be assumed */
    MR::float32 spacing_x;     /*!< spacing in the X direction (ranging [0...width]) - default is 1 */
    MR::float32 spacing_y;     /*!< spacing in the Y direction (ranging [0...height]) - default is 1 */
    MR::float32 spacing_z;     /*!< spacing in the Z direction (ranging [0...depth]) - default is 1 */

    MR::float32 x_r; /* default is -1 */
    MR::float32 x_a; /* default is 0  */
    MR::float32 x_s; /* default is 0  */
    MR::float32 y_r; /* default is 0  */
    MR::float32 y_a; /* default is 0  */
    MR::float32 y_s; /* default is -1 */
    MR::float32 z_r; /* default is 0  */
    MR::float32 z_a; /* default is 1  */
    MR::float32 z_s; /* default is 0  */
    MR::float32 c_r; /* default is 0  */
    MR::float32 c_a; /* default is 0  */
    MR::float32 c_s; /* default is 0  */

    /**** 90 bytes total - note that data starts at 284 ****/

  };

#pragma pack(pop)

  typedef struct mgh_header mgh_header ;


  /*--- the specified data types ---*/
#define MGH_TYPE_UCHAR 0
#define MGH_TYPE_SHORT 4
#define MGH_TYPE_INT   1
#define MGH_TYPE_FLOAT 3

  struct mgh_tag
  {
    int32_t id;
    int64_t size;
    std::string content;
  };

  struct mgh_other
  {
    MR::float32 tr;         /*!< milliseconds */
    MR::float32 flip_angle; /*!< radians */
    MR::float32 te;         /*!< milliseconds */
    MR::float32 ti;         /*!< milliseconds */
    MR::float32 fov;        /*!< IGNORE THIS FIELD (data is inconsistent) */

    std::vector<mgh_tag> tags; /*!< variable length char strings */
  };


  /*=================*/
#ifdef  __cplusplus
}
#endif
/*=================*/

#endif /* _MGH_HEADER_ */
