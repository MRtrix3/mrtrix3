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
#ifndef __sync_syncmanager_h__
#define __sync_syncmanager_h__


#include "gui/mrview/sync/enums.h"
#include "gui/mrview/sync/interprocesssyncer.h"
#include "gui/mrview/window.h"

#include "progressbar.h"
#include "memory.h"
#include "gui/mrview/icons.h"
#include "gui/mrview/window.h"
#include "gui/mrview/mode/list.h"
#include "gui/mrview/tool/list.h"
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
        /**
        * Syncs values from mrview's window, using the interprocess syncer
        */
        class SyncManager :public QObject
        {
          Q_OBJECT

        public:
          SyncManager();
          void SetWindow(MR::GUI::MRView::Window* wind);

        private slots:
          void OnWindowFocusChanged();
          void OnIPSDataReceived(std::vector<std::shared_ptr<QByteArray>> all_messages);

        private:
          MR::GUI::MRView::Window* win;//window we sync with
          InterprocessSyncer* ips;//used to communicate with other processes
          QByteArray ToQByteArray(Eigen::Vector3f data);//conversion utility
          Eigen::Vector3f FromQByteArray(QByteArray vec, unsigned int offset);//conversion utility
          bool SendData(DataKey code, QByteArray data);//sends data to other processes via the ips
        };

      }
    }
  }
}
#endif