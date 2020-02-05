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

#include "gui/mrview/sync/localsocketreader.h"


#include "exception.h" 
#include <vector>
#include <memory> //shared_ptr
namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Sync
      {
        LocalSocketReader::LocalSocketReader(QLocalSocket* mySocket) : QObject(0)
        {
          socket = mySocket;
          QObject::connect(socket, SIGNAL(readyRead()), this, SLOT(OnDataReceived()));
        }

        /**
        * Fires when data is received from another process
        */
        void LocalSocketReader::OnDataReceived()
        {
          std::vector<std::shared_ptr<QByteArray>> messagesReceived;
          while (socket->bytesAvailable() > 0)
          {
            //First byte must always by an unsigned int32 stating how much data to read
            //Wait until we've read all of that
            int loops = 0;
            while (socket->bytesAvailable() < 4)
            {
              loops++;
              socket->waitForReadyRead(1000);//NB that because we are in a slot, readyRead will not be re-emitted
              if (loops > 10)
              {
                DEBUG("OnDataReceived timeout (reading size)");
                return;
              }
            }

            //Read the size of the message that follows
            char fourByteHeader[4];
            socket->read(fourByteHeader, 4);
            unsigned int sizeOfMessage;
            memcpy(&sizeOfMessage, fourByteHeader, 4);


            //Wait until all data is delivered
            loops = 0;
            while (socket->bytesAvailable() < sizeOfMessage)
            {
              loops++;
              socket->waitForReadyRead(1000);
              if (loops > 10)
              {
                DEBUG("OnDataReceived timeout (reading data)");
                return;
              }
            }

            //Read delivered data
            char read[sizeOfMessage];
            socket->read(read, sizeOfMessage);
            std::shared_ptr<QByteArray> readData = std::shared_ptr<QByteArray>(new QByteArray());
            readData->insert(0, read, sizeOfMessage);

            //save message
            messagesReceived.emplace_back(readData);
          }

          //Send a signal with the messages with have read
          emit DataReceived(messagesReceived);
        }

      }
    }
  }
}