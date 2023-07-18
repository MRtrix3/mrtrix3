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

#ifndef __signal_handler_h__
#define __signal_handler_h__

#include <string>

namespace MR
{
  namespace SignalHandler
  {
    //! the type of function expected for on_signal()
    using cleanup_function_type = void(*)();

    //! set up the signal handler
    void init();
    //! add function to be run when a signal is received and at program exit
    void on_signal (cleanup_function_type func);

    //! mark the file for deletion when a signal is received or at program exit
    void mark_file_for_deletion (const std::string& filename);
    //! unmark the file from deletion
    void unmark_file_for_deletion (const std::string& filename);
  }
}

#endif
