/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#ifndef __dwi_tractography_resampling_fixed_step_size_h__
#define __dwi_tractography_resampling_fixed_step_size_h__


#include "dwi/tractography/resampling/resampling.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Resampling {



        class FixedStepSize : public BaseCRTP<FixedStepSize>
        { MEMALIGN(FixedStepSize)

          public:
            FixedStepSize () :
              step_size (value_type(0)) { }

            FixedStepSize (const float ss) :
              step_size (ss) { }

            bool operator() (const Streamline<>&, Streamline<>&) const override;
            bool valid() const override { return step_size; }

            void set_step_size (const value_type ss) { step_size = ss; }
            float get_step_size() const { return step_size; }

          private:
            value_type step_size;

        };



      }
    }
  }
}

#endif



