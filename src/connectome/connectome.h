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



#ifndef __connectome_connectome_h__
#define __connectome_connectome_h__


#include "types.h"


namespace MR {
namespace Connectome {


using node_t = uint32_t;

using matrix_type = Eigen::Matrix<default_type, Eigen::Dynamic, Eigen::Dynamic>;


void verify_matrix (matrix_type&, const node_t);


}
}


#endif

