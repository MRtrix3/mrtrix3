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

#ifndef __surface_types_h__
#define __surface_types_h__


#include "connectome/connectome.h"
#include "surface/polygon.h"



namespace MR
{
  namespace Surface
  {


    using Vertex = Eigen::Vector3d;
    using VertexList = vector<Vertex>;
    using Triangle = Polygon<3>;
    using TriangleList = vector<Triangle>;
    using Quad = Polygon<4>;
    using QuadList = vector<Quad>;

    class Vox : public Eigen::Array3i
    { MEMALIGN (Vox)
      public:
        using Eigen::Array3i::Array3i;
        Vox () : Eigen::Array3i (-1, -1, -1) { }
        Vox (const Eigen::Vector3d& p) : Eigen::Array3i (int(std::round (p[0])), int(std::round (p[1])), int(std::round (p[2]))) { }
        bool operator< (const Vox& i) const
        {
          return ((*this)[2] == i[2] ? (((*this)[1] == i[1]) ? ((*this)[0] < i[0]) : ((*this)[1] < i[1])) : ((*this)[2] < i[2]));
        }
    };

    using label_vector_type = Eigen::Array<Connectome::node_t, Eigen::Dynamic, 1>;



  }
}

#endif

