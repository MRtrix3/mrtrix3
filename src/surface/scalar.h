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


#ifndef __surface_scalar_h__
#define __surface_scalar_h__


#include "surface/mesh.h"


namespace MR
{
  namespace Surface
  {


    class Scalar : public Eigen::Array<default_type, Eigen::Dynamic, 1> 
    { MEMALIGN (Scalar)

      public:
        using Base = Eigen::Array<default_type, Eigen::Dynamic, 1>;

        Scalar (const std::string&, const Mesh&);

        Scalar (const Scalar& that) = default;

        Scalar (Scalar&& that) :
            Base (std::move (that)),
            name (std::move (that.name)) { }

        Scalar() { }

        Scalar& operator= (Scalar&& that) {
          Base::operator= (std::move (that));
          name = std::move (that.name);
          return *this;
        }

        Scalar& operator= (const Scalar& that) {
          Base::operator= (that);
          name = that.name;
          return *this;
        }


        void clear() {
          Base::resize(0);
          name.clear();
        }


        void save (const std::string&) const;

        const std::string& get_name() const { return name; }
        void set_name (const std::string& s) { name = s; }


      private:
        std::string name;

        void load_fs_w    (const std::string&, const Mesh&);
        void load_fs_curv (const std::string&, const Mesh&);

    };



  }
}

#endif

