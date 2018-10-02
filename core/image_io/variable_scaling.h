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


#ifndef __image_handler_variable_scaling_h__
#define __image_handler_variable_scaling_h__

#include "types.h"
#include "image_io/base.h"
#include "file/mmap.h"

namespace MR
{
  namespace ImageIO
  {

    class VariableScaling : public Base
    { NOMEMALIGN
      public:
        VariableScaling (const Header& header) :
          Base (header) { }

        VariableScaling (VariableScaling&&) noexcept = default;
        VariableScaling& operator=(VariableScaling&&) = default;

        class ScaleFactor { NOMEMALIGN
          public:
            default_type offset, scale;
        };

        vector<ScaleFactor> scale_factors;

      protected:
        virtual void load (const Header&, size_t);
        virtual void unload (const Header&);
    };

  }
}

#endif


