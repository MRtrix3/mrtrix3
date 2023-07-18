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

#ifndef __sync_enums_h__
#define __sync_enums_h__

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Sync
      {
        /**
        * The type of message being sent between processes
        */
        enum class MessageKey {
          ConnectedID = 1,
          SyncData = 2 //Data to be synced
        };
        /**
        * For MessageKey::SyncData. This is type of data being sent for syncronising
        */
        enum class DataKey {
          WindowFocus = 1
        };

      }
    }
  }
}
#endif
