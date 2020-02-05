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
#include <QtNetwork>

#include "gui/mrview/sync/client.h"
#include <iostream>


namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Sync
      {
        Client::Client()
        {
          socket = new QLocalSocket();
          SetServerName("mrview_syncer");
        }

        /**
        * Gets the name of the server (localsocketreader) that this is connected to
        */
        QString Client::GetServerName()
        {
          return connectToServerName;
        }

        /**
        *Sets the name of the server (localsocketreader) to connect to
        */
        void Client::SetServerName(QString connectTo)
        {
          connectToServerName = connectTo;
          socket->abort();
        }

        /**
        * Tries to set an outgoing connection to the server
        */
        bool Client::TryConnect()
        {
          socket->abort();
          socket->connectToServer(connectToServerName);
          socket->waitForConnected();
          QLocalSocket::LocalSocketState state = socket->state();

          return state == QLocalSocket::ConnectedState;
        }

        /**
        * Sends data, prefixed with its length (length excludes this 4 bytes header), to the process to which we are connected
        */
        void Client::SendData(QByteArray dat)
        {
          //Prefix data with how long the message is
          QByteArray prefixedData;
          char a[4];
          unsigned int size = (unsigned int)dat.size();
          memcpy(a, &size, 4);
          prefixedData.insert(0, a, 4);
          prefixedData.insert(4, dat, dat.size());

          //send
          socket->write(prefixedData.data(), prefixedData.size());
        }

      }
    }
  }
}