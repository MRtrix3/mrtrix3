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

#include <QCryptographicHash>
#include "gui/mrview/sync/processlock.h"


namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Sync
      {


        QString GenerateKeyHash(const QString& key, const QString& salt)
        {
          QByteArray data;

          data.append(key.toUtf8());
          data.append(salt.toUtf8());
          data = QCryptographicHash::hash(data, QCryptographicHash::Sha1).toHex();

          return data;
        }




        ProcessLock::ProcessLock(const QString& key)
          : key(key)
          , memLockKey(GenerateKeyHash(key, "_memLockKey"))
          , sharedmemKey(GenerateKeyHash(key, "_sharedmemKey"))
          , sharedMem(sharedmemKey)
          , memLock(memLockKey, 1)
        {
          memLock.acquire();
          {
            QSharedMemory fix(sharedmemKey);    // Fix for *nix: http://habrahabr.ru/post/173281/
            fix.attach();
          }
          memLock.release();
        }

        ProcessLock::~ProcessLock()
        {
          Release();
        }

        bool ProcessLock::IsAnotherRunning()
        {
          if (sharedMem.isAttached())
            return false;

          memLock.acquire();
          const bool isRunning = sharedMem.attach();
          if (isRunning)
            sharedMem.detach();
          memLock.release();

          return isRunning;
        }

        bool ProcessLock::TryToRun()
        {
          if (IsAnotherRunning())   // Extra check
            return false;

          memLock.acquire();
          const bool result = sharedMem.create(sizeof(quint64));
          memLock.release();
          if (!result)
          {
            Release();
            return false;
          }

          return true;
        }

        void ProcessLock::Release()
        {
          memLock.acquire();
          if (sharedMem.isAttached())
            sharedMem.detach();
          memLock.release();
        }
      }
    }
  }
}
