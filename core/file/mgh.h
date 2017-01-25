/* Copyright (c) 2008-2017 the MRtrix3 contributors
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


#ifndef _MGH_HEADER_
#define _MGH_HEADER_


#include <vector>
#include <string>

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
    float   spacing_x;     /*!< spacing in the X direction (ranging [0...width]) - default is 1 */
    float   spacing_y;     /*!< spacing in the Y direction (ranging [0...height]) - default is 1 */
    float   spacing_z;     /*!< spacing in the Z direction (ranging [0...depth]) - default is 1 */

    float x_r; /* default is -1 */
    float x_a; /* default is 0  */
    float x_s; /* default is 0  */
    float y_r; /* default is 0  */
    float y_a; /* default is 0  */
    float y_s; /* default is -1 */
    float z_r; /* default is 0  */
    float z_a; /* default is 1  */
    float z_s; /* default is 0  */
    float c_r; /* default is 0  */
    float c_a; /* default is 0  */
    float c_s; /* default is 0  */

    /**** 90 bytes total - note that data starts at 284 ****/

  };

#pragma pack(pop)

  typedef struct mgh_header mgh_header ;


  /*--- the specified data types ---*/
#define MGH_TYPE_UCHAR 0
#define MGH_TYPE_SHORT 4
#define MGH_TYPE_INT   1
#define MGH_TYPE_FLOAT 3


  struct mgh_other {

    float tr;         /*!< milliseconds */
    float flip_angle; /*!< radians */
    float te;         /*!< milliseconds */
    float ti;         /*!< milliseconds */
    float fov;        /*!< IGNORE THIS FIELD (data is inconsistent) */

    std::vector<std::string> tags; /*!< variable length char strings */

  };

  typedef struct mgh_other mgh_other ;


  /*=================*/
#ifdef  __cplusplus
}
#endif
/*=================*/

#endif /* _MGH_HEADER_ */
