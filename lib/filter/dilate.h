/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08, David Raffelt, 08/06/2012

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

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
    class Dilate : public Base
    {

      public:
        Dilate (const Header& in) :
            Base (in),
            npass (1)
        {
          datatype_ = DataType::Bit;
        }

        Dilate (const Header& in, const std::string& message) :
            Base (in, message),
            npass (1)
        {
          datatype_ = DataType::Bit;
        }


        template <class InputImageType, class OutputImageType>
        void operator() (InputImageType& input, OutputImageType& output)
        {
          std::shared_ptr <Image<bool> > in (new Image<bool> (Image<bool>::scratch (input.header())));
          copy (input, *in);
          std::shared_ptr <Image<bool> > out;
          std::shared_ptr<ProgressBar> progress (message.size() ? new ProgressBar (message, npass + 1) : nullptr);

          for (unsigned int pass = 0; pass < npass; pass++) {
            out = std::make_shared<Image<bool>> (Image<bool>::scratch (input.header()));
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
