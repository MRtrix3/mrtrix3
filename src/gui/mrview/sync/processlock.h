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
*
*
* NOTE: this code has been based on a code snippet from  https://stackoverflow.com/questions/5006547/qt-best-practice-for-a-single-instance-app-protection
*
*/
#ifndef __sync_processlock_h__
#define __sync_processlock_h__

#include <QObject>
#include <QSharedMemory>
#include <QSystemSemaphore>

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Sync
      {
        /**
        * Can be used to prevent multiple processes accessing a resource at the same time. QLockFile is another option but not available in Qt 4.8
        * Use TryToRun(), check the returned value on whether to continue or not, then call Release when done.
        */
        class ProcessLock
        {

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