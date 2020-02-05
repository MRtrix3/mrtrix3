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
#ifndef __sync_localsocketreader_h__
#define __sync_localsocketreader_h__

#include <QtNetwork>

#include <qlocalsocket.h>
#include <iostream> //temp
#include <vector>
#include <memory> //shared_ptr
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
        * Auto reads data from its local socket when data arrives, and fires an event with that data attached
        */
        class LocalSocketReader :public QObject
        {
          Q_OBJECT

        public:
          LocalSocketReader(QLocalSocket* mySocket);


        signals:
          void DataReceived(std::vector<std::shared_ptr<QByteArray>> dat);//emits every message currently available


        private slots:
          void OnDataReceived();


        private:
          QLocalSocket * socket;
        };

      }
    }
  }
}
#endif