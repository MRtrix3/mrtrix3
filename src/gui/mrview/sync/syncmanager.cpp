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
#include "gui/mrview/sync/syncmanager.h"
#include "gui/mrview/window.h"
#include <iostream>
#include <vector>
#include <memory> //shared_ptr
#include "gui/mrview/sync/enums.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Sync
      {
        SyncManager::SyncManager() : QObject(0)
        {
          ips = new InterprocessSyncer();
          connect(ips, SIGNAL(SyncDataReceived(std::vector<std::shared_ptr<QByteArray>>)), this, SLOT(OnIPSDataReceived(std::vector<std::shared_ptr<QByteArray>>)));
        }

        /**
        * Sets the window to connect to. Code currently assumes this only occurs once
        */
        void SyncManager::SetWindow(MR::GUI::MRView::Window* wind)
        {
          win = wind;
          connect(win, SIGNAL(focusChanged()), this, SLOT(OnWindowFocusChanged()));
        }

        /**
        * Receives a signal from window that the focus has changed
        */
        void SyncManager::OnWindowFocusChanged()
        {
          if (win->sync_focus_on())
          {
            Eigen::Vector3f foc = win->focus();
            SendData(DataKey::WindowFocus, ToQByteArray(foc));
          }
        }

        /**
        * Sends a signal to other processes to sync to a given key/value
        */
        bool SyncManager::SendData(DataKey code, QByteArray dat)
        {
          QByteArray data;
          char codeAsChar[4];
          InterprocessSyncer::Int32ToChar(codeAsChar, (int)code);
          data.insert(0, codeAsChar, 4);
          data.insert(4, dat, dat.size());

          return ips->SendData(data);
        }

        /**
        * Receives a signal from another process that a value to sync has changed
        */
        void SyncManager::OnIPSDataReceived(std::vector<std::shared_ptr<QByteArray>> all_messages)
        {
          //WARNING This code assumes that the order of syncing operations does not matter

          //We have a list of messages found
          //Categorise these. Only keep the last value sent for each message type, or we will change to an old value and then update other processes to this old value
          std::shared_ptr<QByteArray> winFocus = 0;

          for (size_t i = 0; i < all_messages.size(); i++)
          {
            std::shared_ptr<QByteArray> data = all_messages[i];

            if (data->size() < 4)
            {
              DEBUG("Bad data received to syncmanager: too short");
              continue;
            }

            int idOfDataEntry = InterprocessSyncer::CharTo32bitNum(data->data());

            switch (idOfDataEntry)
            {
              case (int)DataKey::WindowFocus:
              {
                //This message has window focus information to sync with
                winFocus = data;
                break;
              }
              default:
              {
                DEBUG("Unknown data key received: " + idOfDataEntry);
                break;
              }
            }
          }


          if (winFocus && win->sync_focus_on())
          {
            //We received 1+ signals to change our window focus

            unsigned int offset = 4;//we have already read 4 bytes, above
            //Read three single point floats 
            if (winFocus->size() != (int)(offset + 12)) //cast to int to avoid compiler warning
            {
              DEBUG("Bad data received to sync manager: wrong length (window focus)");
            }
            else
            {
              Eigen::Vector3f vec = FromQByteArray(*winFocus, offset);

              //Check if already set to this value - Basic OOP: don't trust window to check things are changed before emitting a signal that the value has changed!
              Eigen::Vector3f win_vec = win->focus();
              if (win_vec[0] != vec[0] || win_vec[1] != vec[1] || win_vec[2] != vec[2])
              {
                //Send to window
                win->set_focus(vec);
              }
            }
          }


          //Redraw the window. 
          win->updateGL();
        }

        /**
        * Serialises a Vector3f as a QByteArray
        */
        QByteArray SyncManager::ToQByteArray(Eigen::Vector3f data)
        {
          char a[12];
          memcpy(a, &data, 12);
          QByteArray q;
          q.insert(0, a, 12); //don't use the constructor; it ignores any data after hitting a \0
          return q;
        }
        /**
        * Deserialises a Vector3f from a QByteArray
        */
        Eigen::Vector3f SyncManager::FromQByteArray(QByteArray data, unsigned int offset)
        {
          Eigen::Vector3f read;
          memcpy(&read, data.data() + offset, 12);
          return read;
        }

      }
    }
  }
}