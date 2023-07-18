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

#include "gui/mrview/window.h"
#include "gui/mrview/sync/syncmanager.h"

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
          try
          {
            ips = new InterprocessCommunicator();//will throw exception if it fails to set up a server
            connect (ips, SIGNAL(SyncDataReceived(vector<std::shared_ptr<QByteArray>>)),
                     this, SLOT(OnIPSDataReceived(vector<std::shared_ptr<QByteArray>>)));
          }
          catch (...)
          {
            ips = 0;
            WARN("Sync set up failed.");
          }
          connect (Window::main, SIGNAL(focusChanged()), this, SLOT(OnWindowFocusChanged()));
        }

        /**
        * Returns true if this is in a state which is not appropriate to connect a window
        */
        bool SyncManager::GetInErrorState()
        {
          return ips == 0;
        }

        /**
        * Receives a signal from window that the focus has changed
        */
        void SyncManager::OnWindowFocusChanged()
        {
          if (Window::main->sync_focus_on())
          {
            Eigen::Vector3f foc = Window::main->focus();
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
          InterprocessCommunicator::Int32ToChar(codeAsChar, (int)code);
          data.insert(0, codeAsChar, 4);
          data.insert(4, dat, dat.size());

          return ips->SendData(data);
        }

        /**
        * Receives a signal from another process that a value to sync has changed
        */
        void SyncManager::OnIPSDataReceived(vector<std::shared_ptr<QByteArray>> all_messages)
        {
          //WARNING This code assumes that the order of syncing operations does not matter

          // We have a list of messages found
          // Categorise these. Only keep the last value sent for each message
          // type, or we will change to an old value and then update other
          // processes to this old value
          std::shared_ptr<QByteArray> winFocus = 0;

          for (size_t i = 0; i < all_messages.size(); i++)
          {
            std::shared_ptr<QByteArray> data = all_messages[i];

            if (data->size() < 4)
            {
              DEBUG("Bad data received to syncmanager: too short");
              continue;
            }

            int idOfDataEntry = InterprocessCommunicator::CharTo32bitNum(data->data());

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
                DEBUG("Unknown data key received: " + std::to_string(idOfDataEntry));
                break;
              }
            }
          }


          if (winFocus && Window::main->sync_focus_on())
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

              // Check if already set to this value - Basic OOP:
              // don't trust window to check things are changed before emitting
              // a signal that the value has changed!
              Eigen::Vector3f win_vec = Window::main->focus();
              if (win_vec[0] != vec[0] || win_vec[1] != vec[1] || win_vec[2] != vec[2])
              {
                //Send to window
                  Window::main->set_focus(vec);
              }
            }
          }


          //Redraw the window.
          Window::main->updateGL();
        }

        /**
        * Serialises a Vector3f as a QByteArray
        */
        QByteArray SyncManager::ToQByteArray(Eigen::Vector3f data)
        {
          char a[12];
          memcpy(a, data.data(), 12);
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
          memcpy(read.data(), data.data() + offset, 12);
          return read;
        }

      }
    }
  }
}
