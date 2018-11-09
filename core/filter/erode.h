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


#ifndef __filter_erode_h__
#define __filter_erode_h__

#include "progressbar.h"
#include "memory.h"
#include "image.h"
#include "image_helpers.h"
#include "algo/copy.h"
#include "algo/loop.h"
#include "filter/base.h"

namespace MR
{
  namespace Filter
  {

    /** \addtogroup Filters
      @{ */

    //! a filter to erode a mask
    /*!
     * Typical usage:
     * \code
     * auto input = Image<bool>::open (argument[0]);
     *
     * Filter::Erode erode (input);
     *
     * Image<bool> output (erode, argument[1]);
     * erode (input, output);
     *
     * \endcode
     */
    class Erode : public Base { MEMALIGN(Erode)

      public:
        template <class HeaderType>
        Erode (const HeaderType& in) :
            Base (in),
            npass (1)
        {
          check_3D_nonunity (in);
          datatype_ = DataType::Bit;
        }

        template <class HeaderType>
        Erode (const HeaderType& in, const std::string& message) :
            Base (in, message),
            npass (1)
        {
          check_3D_nonunity (in);
          datatype_ = DataType::Bit;
        }


        template <class InputImageType, class OutputImageType>
        void operator() (InputImageType& input, OutputImageType& output)
        {
          std::shared_ptr <Image<bool> > in = make_shared<Image<bool> > (Image<bool>::scratch (input));
          copy (input, *in);
          std::shared_ptr <Image<bool> > out;
          std::shared_ptr<ProgressBar> progress (message.size() ? new ProgressBar (message, npass + 1) : nullptr);

          for (unsigned int pass = 0; pass < npass; pass++) {
            out = make_shared<Image<bool> > (Image<bool>::scratch (input));
            for (auto l = Loop (*in) (*in, *out); l; ++l)
             out->value() = erode (*in);

            if (pass < npass - 1)
              in = out;
            if (progress)
              ++(*progress);
          }
          copy (*out, output);
        }


        void set_npass (unsigned int npasses)
        {
          npass = npasses;
        }


      protected:

        bool erode (Image<bool>& in)
        {
          if (!in.value()) return false;
          if (   (in.index(0) == 0) || (in.index(0) == in.size(0)-1)
              || (in.index(1) == 0) || (in.index(1) == in.size(1)-1)
              || (in.index(2) == 0) || (in.index(2) == in.size(2)-1))
            return false;
          bool val;
          if (in.index(0) > 0) { in.index(0)--; val = in.value(); in.index(0)++; if (!val) return false; }
          if (in.index(1) > 0) { in.index(1)--; val = in.value(); in.index(1)++; if (!val) return false; }
          if (in.index(2) > 0) { in.index(2)--; val = in.value(); in.index(2)++; if (!val) return false; }
          if (in.index(0) < in.size(0)-1) { in.index(0)++; val = in.value(); in.index(0)--; if (!val) return false; }
          if (in.index(1) < in.size(1)-1) { in.index(1)++; val = in.value(); in.index(1)--; if (!val) return false; }
          if (in.index(2) < in.size(2)-1) { in.index(2)++; val = in.value(); in.index(2)--; if (!val) return false; }
          return true;
        }

        unsigned int npass;
    };
    //! @}
  }
}




#endif
