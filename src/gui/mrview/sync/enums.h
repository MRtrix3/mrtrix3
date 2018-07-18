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