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


#ifndef __fixel_filter_base_h__
#define __fixel_filter_base_h__

namespace MR
{
  namespace Fixel
  {
    namespace Filter
    {

      /*! A base class for defining fixel data filters.
       *
       * The Fixel::Filter::Base class defines the basic
       * interface for defining filters that operate upon fixel
       * data. It allows these filters to be initialised,
       * set up and run using base class pointers, and defines a
       * standardised functor interface that fixel data filter
       * classes should ideally conform to.
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

          virtual void operator() (Image<float>&, Image<float>&) const
          {
            throw Exception ("Running empty function Fixel::Filter::Base::operator()");
          }

        protected:
          std::string message;

      };
      //! @}



    }
  }
}


#endif
