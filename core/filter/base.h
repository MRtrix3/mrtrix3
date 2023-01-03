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

#ifndef __filter_base_h__
#define __filter_base_h__

#include "progressbar.h" // May be needed for any derived classes that make use of the message string
#include "header.h"

namespace MR
{
  namespace Filter
  {
    /** \addtogroup Filters
    @{ */

    /*! A base class for defining image filters.
     *
     * The Filter::Base class defines the basic interface for
     * defining image filters. Since these filters can vary
     * substantially in their design and implementation, the
     * actual functionality of the Base class is almost zero
     * (above and beyond that of the Header class).
     *
     * It does however allow these filters to be initialised,
     * set up and run using base class pointers, and defines a
     * standardised functor interface that image filter classes
     * should ideally conform to.
     *
     */
    class Base : public Header { MEMALIGN(Base)
      public:
        template <class HeaderType>
        Base (const HeaderType& in) :
            Header (in) { }

        template <class HeaderType>
        Base (const HeaderType& in, const std::string& message) :
            Header (in),
            message (message) { }

        virtual ~Base() { }

        void set_message (const std::string& s) { message = s; }

        template <class InputImageType, class OutputImageType>
        void operator() (InputImageType& in, OutputImageType& out)
        {
            throw Exception ("Running empty function Filter::Base::operator()");
        }

      protected:
        std::string message;

    };
    //! @}
  }
}


#endif
