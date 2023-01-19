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
#ifndef __sync_client_h__
#define __sync_client_h__

#include <qlocalsocket.h>
#include "types.h"

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
        { NOMEMALIGN

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
