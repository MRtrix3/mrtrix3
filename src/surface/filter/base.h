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


#ifndef __surface_filter_base_h__
#define __surface_filter_base_h__

#include "progressbar.h" // May be needed for any derived classes that make use of the message string

#include "surface/mesh.h"
#include "surface/mesh_multi.h"

namespace MR
{
  namespace Surface
  {
    namespace Filter
    {

      /*! A base class for defining surface mesh filters.
       *
       * The Surface::Filter::Base class defines the basic
       * interface for defining filters that operate upon mesh
       * data. It allows these filters to be initialised,
       * set up and run using base class pointers, and defines a
       * standardised functor interface that mesh filter classes
       * should ideally conform to.
       *
       */
      class Base
      { MEMALIGN (Base)
        public:
          Base (const std::string& s) :
              message (s) { }
          Base () { }

          virtual ~Base() { }

          void set_message (const std::string& s) { message = s; }

          virtual void operator() (const Mesh&, Mesh&) const
          {
            throw Exception ("Running empty function Surface::Filter::Base::operator()");
          }

          virtual void operator() (const MeshMulti&, MeshMulti&) const;

        protected:
          std::string message;

      };
      //! @}



    }
  }
}


#endif
