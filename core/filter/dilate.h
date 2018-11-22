/* Copyright (c) 2008-2019 the MRtrix3 contributors.
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
 * See the Mozila Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#ifndef __filter_dilate_h__
#define __filter_dilate_h__

#include "memory.h"
#include "image.h"
#include "algo/copy.h"
#include "algo/loop.h"
#include "filter/base.h"



namespace MR
{
  namespace Filter
  {

    /** \addtogroup Filters
      @{ */

    //! a filter to dilate a mask
    /*!
     * Typical usage:
     * \code
     * auto input = Image<bool>::open (argument[0]);
     *
     * Filter::Dilate dilate (input);
     *
     * Image<bool> output (dilate, argument[1]);
     * dilate (input, output);
     *
     * \endcode
     */
    class Dilate : public Base { MEMALIGN(Dilate)

      public:
        template <class HeaderType>
        Dilate (const HeaderType& in) :
            Base (in),
            npass (1)
        {
          datatype_ = DataType::Bit;
        }

        template <class HeaderType>
        Dilate (const HeaderType& in, const std::string& message) :
            Base (in, message),
            npass (1)
        {
          datatype_ = DataType::Bit;
        }


        template <class InputImageType, class OutputImageType>
        void operator() (InputImageType& input, OutputImageType& output)
        {
          std::shared_ptr <Image<bool> > in (new Image<bool> (Image<bool>::scratch (input)));
          copy (input, *in);
          std::shared_ptr <Image<bool> > out;
          std::shared_ptr<ProgressBar> progress (message.size() ? new ProgressBar (message, npass + 1) : nullptr);

          for (unsigned int pass = 0; pass < npass; pass++) {
            out = make_shared<Image<bool>> (Image<bool>::scratch (input));
            for (auto l = Loop (*in) (*in, *out); l; ++l)
              out->value() = dilate (*in);
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

        bool dilate (Image<bool>& in)
        {
          if (in.value()) return true;
          bool val;
          if (in.index(0) > 0) { in.index(0)--; val = in.value(); in.index(0)++; if (val) return true; }
          if (in.index(1) > 0) { in.index(1)--; val = in.value(); in.index(1)++; if (val) return true; }
          if (in.index(2) > 0) { in.index(2)--; val = in.value(); in.index(2)++; if (val) return true; }
          if (in.index(0) < in.size(0)-1) { in.index(0)++; val = in.value(); in.index(0)--; if (val) return true; }
          if (in.index(1) < in.size(1)-1) { in.index(1)++; val = in.value(); in.index(1)--; if (val) return true; }
          if (in.index(2) < in.size(2)-1) { in.index(2)++; val = in.value(); in.index(2)--; if (val) return true; }
          return false;
        }

        unsigned int npass;
    };
    //! @}
  }
}




#endif
