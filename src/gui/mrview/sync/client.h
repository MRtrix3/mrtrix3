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
#ifndef __sync_client_h__
#define __sync_client_h__

#include <qlocalsocket.h>

class QLocalSocket;


namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Sync
      {
        /**
        Sends data to another process
        */
        class Client
        {

        public:
          Client();
          Client(const Client&) = delete;
          bool TryConnect();
          void SetServerName(QString connectTo);
          QString GetServerName();
          void SendData(QByteArray dat);


        private:
          QString connectToServerName;//the name of the server (localsocketreader) to connect to
          QLocalSocket *socket;
        };

      }
    }
  }
}
#endif