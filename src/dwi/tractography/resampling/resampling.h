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

#ifndef __dwi_tractography_resampling_resampling_h__
#define __dwi_tractography_resampling_resampling_h__


#include "app.h"

#include "dwi/tractography/streamline.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Resampling {



        extern const App::OptionGroup ResampleOption;


        class Base;
        Base* get_resampler();

        using value_type = float;
        using point_type = typename Streamline<>::point_type;

        // cubic interpolation (tension = 0.0) looks 'bulgy' between control points
        constexpr value_type hermite_tension = value_type(0.1);


        class Base
        { NOMEMALIGN
          public:
            Base() { }
            virtual ~Base() { }

            virtual Base* clone() const = 0;
            virtual bool operator() (const Streamline<>&, Streamline<>&) const = 0;
            virtual bool valid () const = 0;

        };

        template <class Derived>
        class BaseCRTP : public Base
        { NOMEMALIGN
          public:
            virtual Base* clone() const {
              return new Derived(static_cast<Derived const&>(*this));
            }
        };




      }
    }
  }
}

#endif



