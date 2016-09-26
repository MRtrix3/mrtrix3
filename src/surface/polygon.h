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

#ifndef __surface_polygon_h__
#define __surface_polygon_h__

#include <assert.h>
#include <initializer_list>
#include <stddef.h>
#include <stdint.h>

namespace MR
{
  namespace Surface
  {


    template <uint32_t vertices = 3>
    class Polygon
    {

      public:

        template <typename T>
        Polygon (const T* const d)
        {
          for (size_t i = 0; i != vertices; ++i)
            indices[i] = d[i];
        }

        template <class C>
        Polygon (const C& d)
        {
          assert (d.size() == vertices);
          for (size_t i = 0; i != vertices; ++i)
            indices[i] = d[i];
        }

        template <typename T>
        Polygon (const std::initializer_list<T> l)
        {
          assert (l.size() == vertices);
          size_t counter = 0;
          for (auto i = l.begin(); i != l.end(); ++i, ++counter)
            indices[counter] = *i;
        }

        Polygon()
        {
          memset (indices, 0, vertices * sizeof (uint32_t));
        }


        uint32_t& operator[] (const size_t i)       { assert (i < vertices); return indices[i]; }
        uint32_t  operator[] (const size_t i) const { assert (i < vertices); return indices[i]; }

        size_t size() const { return vertices; }

        bool shares_edge (const Polygon&) const;

      private:
        uint32_t indices[vertices];

    };

    template <> bool Polygon<3>::shares_edge (const Polygon<3>&) const;



  }
}

#endif

