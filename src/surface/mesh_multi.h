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


#ifndef __surface_mesh_multi_h__
#define __surface_mesh_multi_h__


#include <vector>

#include "surface/mesh.h"



namespace MR
{
  namespace Surface
  {



    // Class to handle multiple meshes per file
    // For now, this will only be supported using the .obj file type
    // TODO Another alternative may be .vtp: XML-based polydata by VTK
    //   (would allow embedding binary data within the file, rather than
    //   everything being ASCII as in .obj)

    class MeshMulti : public vector<Mesh>
    { MEMALIGN (MeshMulti)
      public:
        using vector<Mesh>::vector;

        void load (const std::string&);
        void save (const std::string&) const;
    };



  }
}

#endif

