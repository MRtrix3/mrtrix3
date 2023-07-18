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

#include "gui/mrview/file_open.h"
#include "gui/mrview/window.h"

namespace MR
{
  namespace GUI
  {

    bool App::event (QEvent *event)
    {
      if (event->type() == QEvent::FileOpen) {
        QFileOpenEvent *openEvent = static_cast<QFileOpenEvent *>(event);
        vector<std::unique_ptr<MR::Header>> list;
        try {
          list.push_back (make_unique<MR::Header> (MR::Header::open (openEvent->file().toUtf8().data())));
        }
        catch (Exception& E) {
          E.display();
        }
        reinterpret_cast<MRView::Window*> (main_window)->add_images (list);
      }
      return QApplication::event(event);
    }


  }
}

