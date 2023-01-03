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
#ifndef __sync_processlock_h__
#define __sync_processlock_h__

#include <QSharedMemory>
#include <QSystemSemaphore>

#include "types.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Sync
      {
        /**
        * Can be used to prevent multiple processes accessing a resource at the
        * same time. QLockFile is another option but not available in Qt 4.8
        * Use TryToRun(), check the returned value on whether to continue or
        * not, then call Release when done.
        */
        class ProcessLock
        { NOMEMALIGN

        public:
          ProcessLock(const QString& key);
          ~ProcessLock();

          bool IsAnotherRunning();
          bool TryToRun();
          void Release();

        private:
          const QString key;
          const QString memLockKey;
          const QString sharedmemKey;

          QSharedMemory sharedMem;
          QSystemSemaphore memLock;

          Q_DISABLE_COPY(ProcessLock)
        };

      }
    }
  }
}

#endif // __sync_processlock_h__
