/*
    Copyright 2012 Brain Research Institute, Melbourne, Australia

    Written by Robert Smith, 22/04/2015.

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



#ifndef __connectome_connectome_h__
#define __connectome_connectome_h__


#include "types.h"


namespace MR {
namespace Connectome {


typedef uint32_t node_t;

typedef Eigen::MatrixXd matrix_type;


void verify_matrix (matrix_type& in, const node_t num_nodes);


}
}


#endif

