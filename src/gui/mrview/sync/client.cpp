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