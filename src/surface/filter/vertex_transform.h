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

#ifndef __surface_filter_vertex_transform_h__
#define __surface_filter_vertex_transform_h__

#include "header.h"
#include "transform.h"

#include "surface/mesh.h"
#include "surface/mesh_multi.h"
#include "surface/filter/base.h"



namespace MR
{
  namespace Surface
  {
    namespace Filter
    {


      class VertexTransform : public Base
      { MEMALIGN (VertexTransform)
        public:
          enum class transform_t { UNDEFINED, FIRST2REAL, REAL2FIRST, VOXEL2REAL, REAL2VOXEL };

          VertexTransform (const Header& H) :
              header (H),
              transform (H),
              mode (transform_t::UNDEFINED) { }

          void set_first2real() { mode = transform_t::FIRST2REAL; }
          void set_real2first() { mode = transform_t::REAL2FIRST; }
          void set_voxel2real() { mode = transform_t::VOXEL2REAL; }
          void set_real2voxel() { mode = transform_t::REAL2VOXEL; }

          transform_t get_mode() const { return mode; }

          void operator() (const Mesh&, Mesh&) const override;

          void operator() (const MeshMulti& in, MeshMulti& out) const {
            Base::operator() (in, out);
          }

        private:
          const Header& header;
          Transform transform;
          transform_t mode;

      };


    }
  }
}

#endif

