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
#ifndef __sync_interprocesscommunicator_h__
#define __sync_interprocesscommunicator_h__

#include "gui/mrview/sync/localsocketreader.h"
#include <qlocalsocket.h>
#include "gui/mrview/sync/client.h"
#include <vector>
#include <memory> //shared_ptr


#define MAX_NO_ALLOWED 32 //maximum number of interprocess syncers that are allowed

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
        {
          Q_OBJECT

        public:
          InterprocessCommunicator();
          static void Int32ToChar(char a[], int n);
          static int CharTo32bitNum(char a[]);
          bool SendData(QByteArray data);//sends data to be synced

        private slots:
          void OnNewIncomingConnection();
          void OnDataReceived(std::vector<std::shared_ptr<QByteArray>> dat);

        signals:
          void SyncDataReceived(std::vector<std::shared_ptr<QByteArray>> data); //fires when data is received which is for syncing. It is up to listeners to validate and store this value

        private:
          int id;//id which is unique between mrview processes
          std::vector<std::shared_ptr<GUI::MRView::Sync::Client>> senders;//send information
          QLocalServer *receiver;//listens for information
          void TryConnectTo(int connectToId);//tries to connect with another interprocesscommunicator


        };

      }
    }
  }
}
#endif