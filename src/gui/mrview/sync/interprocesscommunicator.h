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
#ifndef __sync_interprocesscommunicator_h__
#define __sync_interprocesscommunicator_h__

#include <qlocalsocket.h>

#include "gui/mrview/sync/localsocketreader.h"
#include "gui/mrview/sync/client.h"


// maximum number of inter process syncers that are allowed. This can be
// raised, but may reduce performance when new IPS are created.
#define MAX_NO_ALLOWED 32

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Sync
      {

        /**
        * Sends and receives information from other MRView processes
        */
        class InterprocessCommunicator : public QObject
        { NOMEMALIGN
          Q_OBJECT

        public:
          InterprocessCommunicator();
          static void Int32ToChar(char a[], int n);
          static int CharTo32bitNum(char a[]);
          bool SendData(QByteArray data);//sends data to be synced

        private slots:
          void OnNewIncomingConnection();
          void OnDataReceived(vector<std::shared_ptr<QByteArray>> dat);

        signals:
          void SyncDataReceived(vector<std::shared_ptr<QByteArray>> data); //fires when data is received which is for syncing. It is up to listeners to validate and store this value

        private:
          int id;//id which is unique between mrview processes
          vector<std::shared_ptr<GUI::MRView::Sync::Client>> senders;//send information
          QLocalServer *receiver;//listens for information
          void TryConnectTo(int connectToId);//tries to connect with another interprocesscommunicator


        };

      }
    }
  }
}
#endif
